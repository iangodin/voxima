#!/usr/bin/env python3
"""
Test to verify word boundaries are returned from the server
"""

import asyncio
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).parent))

from edge_tts_server import TTSServer
from tts_client import TTSClient


async def test_boundaries():
    """Test that word boundaries are returned"""
    
    # Start server
    print("Starting TTS server...")
    server = TTSServer('127.0.0.1', 5555)
    server_task = asyncio.create_task(server.start())
    
    # Wait for startup
    await asyncio.sleep(2)
    
    print("Testing word boundaries...\n")
    
    text = "The quick brown fox jumps over the lazy dog"
    
    client = TTSClient('127.0.0.1', 5555)
    result = await client.synthesize(
        text=text,
        speaker="en-US-AriaNeural",
        language="en-US"
    )
    
    print("=" * 70)
    print("RESULT")
    print("=" * 70)
    
    if result.success:
        print(f"✓ Synthesis successful")
        print(f"  Audio size: {len(result.audio_data)} bytes")
        print(f"  Duration: {result.duration_ms}ms")
        print(f"  Word boundaries: {len(result.boundaries)} words")
        
        if result.boundaries:
            print("\nWord Boundaries:")
            print("-" * 70)
            for i, boundary in enumerate(result.boundaries, 1):
                ms = boundary.offset
                word = boundary.text
                bar_length = ms // 100
                bar = '█' * min(bar_length, 50)
                print(f"  {i:2d}. {ms:5d}ms {bar:50s} {word}")
            print("-" * 70)
        else:
            print("\n⚠ WARNING: No word boundaries received!")
            print("  This means the server isn't extracting boundaries properly.")
    else:
        print(f"✗ Synthesis failed: {result.message}")
    
    # Shutdown
    server_task.cancel()
    try:
        await server_task
    except asyncio.CancelledError:
        pass


if __name__ == '__main__':
    asyncio.run(test_boundaries())
