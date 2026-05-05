#!/usr/bin/env python3
"""
Standalone concurrent test - starts server and tests it
"""

import asyncio
import sys
import time
from pathlib import Path

# Add current directory to path
sys.path.insert(0, str(Path(__file__).parent))

from edge_tts_server import TTSServer
from tts_client import TTSClient


async def test_concurrent():
    """Test concurrent requests"""

    # Start server in background
    print("Starting TTS server...")
    server = TTSServer('127.0.0.1', 5555)
    server_task = asyncio.create_task(server.start())

    # Wait for server to start
    await asyncio.sleep(2)

    print("Server started. Testing concurrent requests...\n")

    requests = [
        ("Hello", "en-US-AriaNeural", "en-US"),
        ("Bonjour", "fr-FR-DeniseNeural", "fr-FR"),
        ("Hola", "es-ES-AlvaroNeural", "es-ES"),
        ("Ciao", "it-IT-IsabellaNeural", "it-IT"),
        ("Hallo", "de-DE-ConradNeural", "de-DE"),
    ]

    print("Creating 5 concurrent synthesis requests...\n")
    start = time.time()

    # Create clients for each request (each gets its own connection)
    tasks = []
    for text, speaker, lang in requests:
        client = TTSClient('127.0.0.1', 5555)
        tasks.append(
            client.synthesize(text=text, speaker=speaker, language=lang)
        )

    # Run all concurrently
    results = await asyncio.gather(*tasks, return_exceptions=True)

    elapsed = time.time() - start

    # Print results
    print("=" * 70)
    print("RESULTS")
    print("=" * 70)

    success_count = 0
    for (text, speaker, _), result in zip(requests, results):
        if isinstance(result, Exception):
            print(f"✗ {text:15} ({speaker:30}) - Exception: {result}")
        elif result.success:
            print(f"✓ {text:15} ({speaker:30}) - {len(result.audio_data)} bytes")
            success_count += 1
        else:
            print(f"✗ {text:15} ({speaker:30}) - {result.message}")

    print("=" * 70)
    print(f"Success: {success_count}/{len(requests)} requests")
    print(f"Time: {elapsed:.2f}s")
    print("=" * 70)

    # Shutdown
    server_task.cancel()
    try:
        await server_task
    except asyncio.CancelledError:
        pass

    return success_count == len(requests)


if __name__ == '__main__':
    try:
        success = asyncio.run(test_concurrent())
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\nInterrupted")
        sys.exit(1)
