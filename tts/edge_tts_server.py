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
from dataclasses import dataclass, asdict
from typing import Optional
import socket

try:
    import edge_tts
except ImportError:
    print("ERROR: edge_tts not installed. Install with: pip install edge-tts")
    sys.exit(1)

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


@dataclass
class TTSResponse:
    """Structure for TTS response data"""
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
    ) -> TTSResponse:
        """
        Synthesize speech and extract word boundaries

        Args:
            text: Text to synthesize
            speaker: Voice speaker name (e.g., 'en-US-AriaNeural')
            language: Language code (e.g., 'en-US')
            rate: Speech rate multiplier (0.5 to 2.0)

        Returns:
            TTSResponse with audio data and word boundaries
        """
        try:
            if not text or not text.strip():
                return TTSResponse(
                    status="error",
                    message="Text cannot be empty"
                )

            # Validate speaker exists
            if self.voice_list:
                valid_speakers = [v['ShortName'] for v in self.voice_list]
                if speaker not in valid_speakers:
                    return TTSResponse(
                        status="error",
                        message=f"Speaker '{speaker}' not found. Available speakers: {', '.join(valid_speakers[:5])}..."
                    )

            # Synthesize audio
            audio_data = bytearray()
            boundaries = []

            # CRITICAL: Set boundary="WordBoundary" to get word-level timestamps
            # Default is "SentenceBoundary" which gives sentence-level boundaries instead
            communicate = edge_tts.Communicate(
                text=text,
                voice=speaker,
                rate=f"{rate:+.0%}",  # Format as +0% to -50%
                pitch="+0Hz",
                boundary="WordBoundary"  # Request word-level boundaries, not sentence-level
            )

            async for chunk in communicate.stream():
                if chunk['type'] == 'audio':
                    audio_data.extend(chunk['data'])
                elif chunk['type'] == 'WordBoundary':
                    # edge_tts WordBoundary chunk structure:
                    # {
                    #   'type': 'WordBoundary',
                    #   'offset': <milliseconds>,
                    #   'duration': <duration_in_ms>,
                    #   'text': <word_text>
                    # }
                    boundaries.append({
                        'text': chunk.get('text', ''),
                        'offset': chunk.get('offset', 0),
                        'duration': chunk.get('duration', 0)
                    })

            if not audio_data:
                return TTSResponse(
                    status="error",
                    message="No audio data generated"
                )

            # Estimate duration from last boundary offset
            duration_ms = boundaries[-1]['offset'] + boundaries[-1]['duration'] if boundaries else 0

            return TTSResponse(
                status="success",
                message="Synthesis completed",
                audio_data=bytes(audio_data),
                boundaries=boundaries,
                duration_ms=duration_ms
            )

        except Exception as e:
            logger.error(f"Synthesis error: {e}", exc_info=True)
            return TTSResponse(
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

                # Sanity check for message size (max 1MB)
                if message_length > 1024 * 1024:
                    logger.warning(f"Message too large: {message_length} bytes")
                    try:
                        await self.send_response(
                            writer,
                            TTSResponse(
                                status="error",
                                message="Message too large (max 1MB)"
                            )
                        )
                    except Exception:
                        break
                    continue

                # Read the actual message
                message_data = await reader.readexactly(message_length)

                try:
                    request = json.loads(message_data.decode('utf-8'))
                    response = await self.process_request(request)
                except json.JSONDecodeError as e:
                    response = TTSResponse(
                        status="error",
                        message=f"Invalid JSON: {str(e)}"
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

    async def process_request(self, request: dict) -> TTSResponse:
        """Process a TTS synthesis request"""
        try:
            # Validate required fields
            if 'text' not in request:
                return TTSResponse(status="error", message="Missing 'text' field")
            if 'speaker' not in request:
                return TTSResponse(status="error", message="Missing 'speaker' field")
            if 'language' not in request:
                return TTSResponse(status="error", message="Missing 'language' field")

            text = request['text']
            speaker = request['speaker']
            language = request['language']
            rate = request.get('rate', 1.0)

            # Validate inputs
            if not isinstance(text, str) or not text.strip():
                return TTSResponse(status="error", message="Text must be non-empty string")
            if not isinstance(speaker, str):
                return TTSResponse(status="error", message="Speaker must be string")
            if not isinstance(language, str):
                return TTSResponse(status="error", message="Language must be string")
            if not isinstance(rate, (int, float)) or rate < 0.5 or rate > 2.0:
                return TTSResponse(status="error", message="Rate must be between 0.5 and 2.0")

            # Truncate text to reasonable length
            if len(text) > 10000:
                text = text[:10000]
                logger.warning("Text truncated to 10000 characters")

            # Synthesize
            response = await self.bridge.synthesize(text, speaker, language, rate)
            return response

        except Exception as e:
            logger.error(f"Request processing error: {e}")
            return TTSResponse(
                status="error",
                message=f"Processing error: {str(e)}"
            )

    async def send_response(self, writer, response: TTSResponse):
        """Send response back to client"""
        try:
            # Build response object
            response_obj = {
                'status': response.status,
                'message': response.message,
            }

            if response.boundaries is not None:
                response_obj['boundaries'] = response.boundaries

            if response.duration_ms is not None:
                response_obj['duration_ms'] = response.duration_ms

            # Handle audio data
            if response.audio_data:
                # Encode audio as base64 for JSON
                import base64
                response_obj['audio_base64'] = base64.b64encode(response.audio_data).decode('utf-8')

            # Serialize to JSON
            response_json = json.dumps(response_obj, ensure_ascii=False).encode('utf-8')

            # Send length prefix + data
            length = struct.pack('!I', len(response_json))
            writer.write(length + response_json)
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
