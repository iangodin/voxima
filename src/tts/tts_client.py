#!/usr/bin/env python3
"""
Edge TTS Bridge Client - Connect to the TTS server and get audio + word boundaries
"""

import asyncio
import struct
import os
from dataclasses import dataclass
from typing import Optional, List
import socket

try:
    import capnp
except ImportError:
    print("ERROR: pycapnp not installed. Install with: pip install pycapnp")
    import sys
    sys.exit(1)

# Load Cap'n Proto schema
try:
    SCHEMA_PATH = os.path.join(os.path.dirname(__file__), 'protocol.capnp')
    protocol = capnp.load(SCHEMA_PATH)
except Exception as e:
    print(f"ERROR: Failed to load Cap'n Proto schema from {SCHEMA_PATH}: {e}")
    import sys
    sys.exit(1)


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
    """Client for connecting to Edge TTS Bridge Server using Cap'n Proto"""

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
            # Build Cap'n Proto request
            request = protocol.TTSRequest.new_message()
            request.text = text
            request.speaker = speaker
            request.language = language
            request.rate = float(rate)

            # Send request
            request_bytes = request.to_bytes()
            length = struct.pack('!I', len(request_bytes))
            self.writer.write(length + request_bytes)
            await self.writer.drain()

            # Receive response
            length_data = await self.reader.readexactly(4)
            response_length = struct.unpack('!I', length_data)[0]

            response_data = await self.reader.readexactly(response_length)
            
            # Parse Cap'n Proto response
            with protocol.TTSResponse.from_bytes(response_data) as response:
                # Parse boundaries
                boundaries = None
                if len(response.boundaries) > 0:
                    boundaries = [
                        WordBoundary(text=b.text, offset=b.offset, duration=b.duration)
                        for b in response.boundaries
                    ]

                result = SynthesisResult(
                    status=response.status,
                    message=response.message,
                    audio_data=response.audio,
                    boundaries=boundaries,
                    duration_ms=response.duration if response.duration != 0 else None
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
                print(f"  {boundary.offset:4d}ms: {boundary.text}")

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

