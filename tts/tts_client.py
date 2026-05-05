#!/usr/bin/env python3
"""
Edge TTS Bridge Client - Connect to the TTS server and get audio + word boundaries
"""

import asyncio
import base64
import json
import struct
from dataclasses import dataclass
from typing import Optional, List
import socket


@dataclass
class WordBoundary:
    """Word boundary with timestamp"""
    text: str
    offset: int
    duration: int


@dataclass
class SynthesisResult:
    """Result from TTS synthesis"""
    status: str
    message: str
    audio_data: Optional[bytes] = None
    boundaries: Optional[List[WordBoundary]] = None
    duration_ms: Optional[int] = None

    @property
    def success(self) -> bool:
        return self.status == "success"


class TTSClient:
    """Client for connecting to Edge TTS Bridge Server"""

    def __init__(self, host: str = '127.0.0.1', port: int = 5555):
        """
        Initialize TTS client

        Args:
            host: Server hostname/IP
            port: Server port
        """
        self.host = host
        self.port = port
        self.reader = None
        self.writer = None
        self._persistent = False  # Track if connection should persist

    async def connect(self):
        """Connect to the TTS server"""
        self.reader, self.writer = await asyncio.open_connection(self.host, self.port)

    async def close(self):
        """Close connection to server"""
        if self.writer:
            self.writer.close()
            try:
                await self.writer.wait_closed()
            except Exception:
                pass
            self.reader = None
            self.writer = None

    async def synthesize(
        self,
        text: str,
        speaker: str,
        language: str,
        rate: float = 1.0,
        _reuse_connection: bool = False
    ) -> SynthesisResult:
        """
        Request speech synthesis

        Args:
            text: Text to synthesize
            speaker: Voice speaker name (e.g., 'en-US-AriaNeural')
            language: Language code (e.g., 'en-US')
            rate: Speech rate multiplier (0.5 to 2.0)
            _reuse_connection: Internal flag - if False, creates new connection per request

        Returns:
            SynthesisResult with audio and word boundaries
        """
        # For concurrent requests, each should use its own connection
        # Context manager users can set _persistent=True for reuse
        use_persistent = _reuse_connection or self._persistent

        if not use_persistent or not self.reader or not self.writer:
            try:
                await self.connect()
            except Exception as e:
                return SynthesisResult(
                    status='error',
                    message=f'Connection failed: {str(e)}'
                )

        try:
            # Build request
            request = {
                'text': text,
                'speaker': speaker,
                'language': language,
                'rate': rate
            }

            # Send request
            request_json = json.dumps(request, ensure_ascii=False).encode('utf-8')
            length = struct.pack('!I', len(request_json))
            self.writer.write(length + request_json)
            await self.writer.drain()

            # Receive response
            length_data = await self.reader.readexactly(4)
            response_length = struct.unpack('!I', length_data)[0]

            response_data = await self.reader.readexactly(response_length)
            response = json.loads(response_data.decode('utf-8'))

            # Parse response
            audio_data = None
            if 'audio_base64' in response:
                audio_data = base64.b64decode(response['audio_base64'])

            boundaries = None
            if 'boundaries' in response:
                boundaries = [
                    WordBoundary(text=b['text'], offset=b['offset'], duration=b['duration'])
                    for b in response['boundaries']
                ]

            result = SynthesisResult(
                status=response['status'],
                message=response['message'],
                audio_data=audio_data,
                boundaries=boundaries,
                duration_ms=response.get('duration_ms')
            )

            # Close connection if not persistent
            if not use_persistent:
                await self.close()

            return result

        except Exception as e:
            await self.close()
            return SynthesisResult(
                status='error',
                message=f'Client error: {str(e)}'
            )

    async def __aenter__(self):
        self._persistent = True
        await self.connect()
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.close()
        self._persistent = False


# Synchronous wrapper for easier use
class TTSClientSync:
    """Synchronous wrapper around TTSClient"""

    def __init__(self, host: str = '127.0.0.1', port: int = 5555):
        self.host = host
        self.port = port

    def synthesize(
        self,
        text: str,
        speaker: str,
        language: str,
        rate: float = 1.0
    ) -> SynthesisResult:
        """Synchronous synthesis request"""
        return asyncio.run(
            self._synthesize_async(text, speaker, language, rate)
        )

    async def _synthesize_async(self, text, speaker, language, rate):
        async with TTSClient(self.host, self.port) as client:
            return await client.synthesize(text, speaker, language, rate)


# Example usage
async def example_async():
    """Example using async client"""
    async with TTSClient('127.0.0.1', 5555) as client:
        result = await client.synthesize(
            text="Hello, this is a test of the speech synthesis system.",
            speaker="en-US-AriaNeural",
            language="en-US",
            rate=1.0
        )

        if result.success:
            print(f"✓ Synthesis successful ({len(result.audio_data)} bytes)")
            print(f"Duration: {result.duration_ms}ms")
            print("\nWord boundaries:")
            for boundary in result.boundaries:
                print(f"  {boundary.milliseconds:4d}ms: {boundary.text}")

            # Save audio to file
            with open('/tmp/output.wav', 'wb') as f:
                f.write(result.audio_data)
            print("\nAudio saved to /tmp/output.wav")
        else:
            print(f"✗ Error: {result.message}")


def example_sync():
    """Example using synchronous client"""
    client = TTSClientSync('127.0.0.1', 5555)

    result = client.synthesize(
        text="Hello, this is a test of the speech synthesis system.",
        speaker="en-US-AriaNeural",
        language="en-US"
    )

    if result.success:
        print(f"✓ Synthesis successful ({len(result.audio_data)} bytes)")
        with open('/tmp/output.wav', 'wb') as f:
            f.write(result.audio_data)
    else:
        print(f"✗ Error: {result.message}")


if __name__ == '__main__':
    import sys

    if len(sys.argv) > 1 and sys.argv[1] == 'async':
        asyncio.run(example_async())
    else:
        example_sync()
