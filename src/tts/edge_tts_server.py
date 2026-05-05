#!/usr/bin/env python3
"""
Edge TTS Bridge Server - TCP socket server for offline speech synthesis
Accepts JSON requests and returns audio + word boundary data
"""

import asyncio
import json
import logging
import struct
import sys
import os
from dataclasses import dataclass, asdict
from typing import Optional
import socket

try:
    import edge_tts
except ImportError:
    print("ERROR: edge_tts not installed. Install with: pip install edge-tts")
    sys.exit(1)

try:
    import capnp
except ImportError:
    print("ERROR: pycapnp not installed. Install with: pip install pycapnp")
    sys.exit(1)

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# Load Cap'n Proto schema
try:
    SCHEMA_PATH = os.path.join(os.path.dirname(__file__), 'protocol.capnp')
    protocol = capnp.load(SCHEMA_PATH)
except Exception as e:
    logger.error(f"Failed to load Cap'n Proto schema from {SCHEMA_PATH}: {e}")
    sys.exit(1)


@dataclass
class TTSResponseData:
    """Internal structure for TTS response data"""
    status: str
    message: str
    audio_data: Optional[bytes] = None
    boundaries: Optional[list] = None
    duration_ms: Optional[int] = None


class EdgeTTSBridge:
    """Bridge to edge_tts with word boundary extraction"""

    def __init__(self):
        self.voice_list = None

    async def initialize(self):
        """Load available voices from edge_tts"""
        try:
            self.voice_list = await edge_tts.list_voices()
            logger.info(f"Loaded {len(self.voice_list)} available voices")
        except Exception as e:
            logger.error(f"Failed to load voices: {e}")
            self.voice_list = []

    async def synthesize(
        self,
        text: str,
        speaker: str,
        language: str,
        rate: float = 1.0
    ) -> TTSResponseData:
        """
        Synthesize speech and extract word boundaries

        Args:
            text: Text to synthesize
            speaker: Voice speaker name (e.g., 'en-US-AriaNeural')
            language: Language code (e.g., 'en-US')
            rate: Speech rate multiplier (0.5 to 2.0)

        Returns:
            TTSResponseData with audio data and word boundaries
        """
        try:
            if not text or not text.strip():
                return TTSResponseData(
                    status="error",
                    message="Text cannot be empty"
                )

            # Validate speaker exists
            if self.voice_list:
                valid_speakers = [v['ShortName'] for v in self.voice_list]
                if speaker not in valid_speakers:
                    return TTSResponseData(
                        status="error",
                        message=f"Speaker '{speaker}' not found. Available speakers: {', '.join(valid_speakers[:5])}..."
                    )

            # Synthesize audio
            audio_data = bytearray()
            boundaries = []

            # CRITICAL: Set boundary="WordBoundary" to get word-level timestamps
            # Default is "SentenceBoundary" which gives sentence-level boundaries instead
            # rate is a multiplier (0.5 to 2.0). edge_tts expects offset from 1.0 (e.g. 1.5 -> +50%)
            rate_offset = rate - 1.0
            communicate = edge_tts.Communicate(
                text=text,
                voice=speaker,
                rate=f"{rate_offset:+.0%}",  # Format as +50% or -50%
                pitch="+0Hz",
                boundary="WordBoundary"  # Request word-level boundaries, not sentence-level
            )

            async for chunk in communicate.stream():
                if chunk['type'] == 'audio':
                    audio_data.extend(chunk['data'])
                elif chunk['type'] == 'WordBoundary':
                    # edge_tts WordBoundary chunk structure:
                    # offset and duration are in 100ns units (ticks)
                    # Convert to milliseconds
                    boundaries.append({
                        'text': chunk.get('text', ''),
                        'offset': chunk.get('offset', 0) // 10000,
                        'duration': chunk.get('duration', 0) // 10000
                    })

            if not audio_data:
                return TTSResponseData(
                    status="error",
                    message="No audio data generated"
                )

            # Estimate duration from last boundary offset
            duration_ms = boundaries[-1]['offset'] + boundaries[-1]['duration'] if boundaries else 0

            return TTSResponseData(
                status="success",
                message="Synthesis completed",
                audio_data=bytes(audio_data),
                boundaries=boundaries,
                duration_ms=duration_ms
            )

        except Exception as e:
            logger.error(f"Synthesis error: {e}", exc_info=True)
            return TTSResponseData(
                status="error",
                message=f"Synthesis failed: {str(e)}"
            )

    def get_voices(self) -> list:
        """Get list of available voices"""
        if not self.voice_list:
            return []
        return self.voice_list


class TTSServer:
    """TCP server for TTS requests"""

    def __init__(self, host: str = '127.0.0.1', port: int = 5555):
        self.host = host
        self.port = port
        self.bridge = EdgeTTSBridge()
        self.server = None

    async def start(self):
        """Start the TCP server"""
        await self.bridge.initialize()

        self.server = await asyncio.start_server(
            self.handle_client,
            self.host,
            self.port
        )

        addr = self.server.sockets[0].getsockname()
        logger.info(f"TTS Server listening on {addr[0]}:{addr[1]}")

        async with self.server:
            await self.server.serve_forever()

    async def handle_client(self, reader, writer):
        """Handle incoming client connections"""
        client_addr = writer.get_extra_info('peername')
        logger.info(f"Client connected: {client_addr}")

        try:
            while True:
                # Read message length (4 bytes, big-endian)
                length_data = await reader.readexactly(4)
                if not length_data:
                    break

                message_length = struct.unpack('!I', length_data)[0]

                # Sanity check for message size (max 10MB for audio/text)
                if message_length > 10 * 1024 * 1024:
                    logger.warning(f"Message too large: {message_length} bytes")
                    try:
                        await self.send_response(
                            writer,
                            TTSResponseData(
                                status="error",
                                message="Message too large (max 10MB)"
                            )
                        )
                    except Exception:
                        break
                    continue

                # Read the actual message
                message_data = await reader.readexactly(message_length)

                try:
                    # Parse Cap'n Proto request
                    with protocol.TTSRequest.from_bytes(message_data) as request:
                        response = await self.process_request(request)
                except Exception as e:
                    logger.error(f"Failed to parse request: {e}")
                    response = TTSResponseData(
                        status="error",
                        message=f"Invalid Cap'n Proto request: {str(e)}"
                    )

                try:
                    await self.send_response(writer, response)
                except (BrokenPipeError, OSError, ConnectionResetError):
                    logger.debug(f"Client {client_addr} connection lost during send")
                    break

        except asyncio.IncompleteReadError:
            logger.debug(f"Client {client_addr} disconnected")
        except (BrokenPipeError, OSError, ConnectionResetError) as e:
            logger.debug(f"Client {client_addr} connection error: {e}")
        except Exception as e:
            logger.error(f"Error handling client {client_addr}: {e}")
        finally:
            try:
                writer.close()
                await writer.wait_closed()
            except Exception:
                pass
            logger.info(f"Client disconnected: {client_addr}")

    async def process_request(self, request) -> TTSResponseData:
        """Process a TTS synthesis request"""
        try:
            text = request.text
            speaker = request.speaker
            language = request.language
            rate = request.rate if request.rate != 0 else 1.0

            # Validate inputs
            if not text or not text.strip():
                return TTSResponseData(status="error", message="Text must be non-empty string")
            
            # Truncate text to reasonable length
            if len(text) > 10000:
                text = text[:10000]
                logger.warning("Text truncated to 10000 characters")

            if rate < 0.1 or rate > 5.0: # Wider range for safety, edge_tts handles it
                return TTSResponseData(status="error", message="Rate must be between 0.1 and 5.0")

            # Synthesize
            response = await self.bridge.synthesize(text, speaker, language, rate)
            return response

        except Exception as e:
            logger.error(f"Request processing error: {e}")
            return TTSResponseData(
                status="error",
                message=f"Processing error: {str(e)}"
            )

    async def send_response(self, writer, response: TTSResponseData):
        """Send response back to client using Cap'n Proto"""
        try:
            # Create Cap'n Proto response
            res_msg = protocol.TTSResponse.new_message()
            res_msg.status = response.status
            res_msg.message = response.message
            
            if response.audio_data:
                res_msg.audio = response.audio_data
            
            if response.duration_ms is not None:
                res_msg.duration = int(response.duration_ms)

            if response.boundaries:
                boundaries_list = res_msg.init('boundaries', len(response.boundaries))
                for i, b in enumerate(response.boundaries):
                    boundaries_list[i].text = b['text']
                    boundaries_list[i].offset = int(b['offset'])
                    boundaries_list[i].duration = int(b['duration'])

            # Serialize to bytes
            response_bytes = res_msg.to_bytes()

            # Send length prefix + data
            length = struct.pack('!I', len(response_bytes))
            writer.write(length + response_bytes)
            await writer.drain()

        except (BrokenPipeError, OSError, ConnectionResetError):
            raise
        except Exception as e:
            logger.error(f"Error sending response: {e}")



async def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(description='Edge TTS TCP Bridge Server')
    parser.add_argument('--host', default='127.0.0.1', help='Bind address (default: 127.0.0.1)')
    parser.add_argument('--port', type=int, default=5555, help='Bind port (default: 5555)')

    args = parser.parse_args()

    server = TTSServer(host=args.host, port=args.port)

    try:
        await server.start()
    except KeyboardInterrupt:
        logger.info("Server shutting down...")
        if server.server:
            server.server.close()


if __name__ == '__main__':
    asyncio.run(main())
