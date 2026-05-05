#!/usr/bin/env python3
"""
Test and example scripts for Edge TTS Bridge Server
"""

import asyncio
import json
import sys
from pathlib import Path
from typing import List

# Import the client
try:
    from tts_client import TTSClient, TTSClientSync, SynthesisResult
except ImportError:
    print("Error: tts_client.py not found in the same directory")
    sys.exit(1)


class Colors:
    """ANSI color codes"""
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'
    BOLD = '\033[1m'


def print_success(msg: str):
    print(f"{Colors.GREEN}✓{Colors.RESET} {msg}")


def print_error(msg: str):
    print(f"{Colors.RED}✗{Colors.RESET} {msg}")


def print_info(msg: str):
    print(f"{Colors.BLUE}ℹ{Colors.RESET} {msg}")


def print_header(msg: str):
    print(f"\n{Colors.BOLD}{Colors.BLUE}{'='*60}{Colors.RESET}")
    print(f"{Colors.BOLD}{msg}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.BLUE}{'='*60}{Colors.RESET}\n")


async def test_basic_synthesis():
    """Test basic synthesis"""
    print_header("Test 1: Basic Synthesis")

    try:
        async with TTSClient('127.0.0.1', 5555) as client:
            result = await client.synthesize(
                text="Hello world, this is a test of the speech synthesis system.",
                speaker="en-US-AriaNeural",
                language="en-US"
            )

            if result.success:
                print_success(f"Synthesis completed")
                print(f"  Audio size: {len(result.audio_data)} bytes")
                print(f"  Duration: {result.duration_ms}ms")
                print(f"  Boundaries: {len(result.boundaries)} words")
                return True
            else:
                print_error(f"Synthesis failed: {result.message}")
                return False
    except Exception as e:
        print_error(f"Connection failed: {e}")
        return False


async def test_word_boundaries():
    """Test word boundary extraction"""
    print_header("Test 2: Word Boundaries")

    try:
        async with TTSClient('127.0.0.1', 5555) as client:
            result = await client.synthesize(
                text="The quick brown fox jumps over the lazy dog.",
                speaker="en-US-GuyNeural",
                language="en-US"
            )

            if result.success:
                print_success("Boundaries extracted")
                print("\nWord timeline:")
                for boundary in result.boundaries:
                    bar_length = boundary.offset // 500000
                    bar = '█' * bar_length
                    print(f"  {boundary.offset:4d}ms {bar} {boundary.text}")
                return True
            else:
                print_error(f"Failed: {result.message}")
                return False
    except Exception as e:
        print_error(f"Error: {e}")
        return False


async def test_multiple_voices():
    """Test different voices"""
    print_header("Test 3: Multiple Voices")

    voices = [
        ("en-US-AriaNeural", "en-US", "American female"),
        ("en-US-GuyNeural", "en-US", "American male"),
        ("en-GB-SoniaNeural", "en-GB", "British female"),
        ("es-ES-AlvaroNeural", "es-ES", "Spanish male"),
    ]

    text = "Welcome to the speech synthesis test."
    results = {}

    async with TTSClient('127.0.0.1', 5555) as client:
        for speaker, lang, description in voices:
            try:
                result = await client.synthesize(
                    text=text,
                    speaker=speaker,
                    language=lang
                )

                if result.success:
                    results[description] = result
                    print_success(f"{description:30} ({len(result.audio_data)} bytes)")
                else:
                    print_error(f"{description:30} - {result.message}")
            except Exception as e:
                print_error(f"{description:30} - {e}")

    return len(results) > 0


async def test_speech_rates():
    """Test different speech rates"""
    print_header("Test 4: Speech Rates")

    rates = [0.5, 0.75, 1.0, 1.25, 1.5]
    text = "Adjust the speech rate to your preference."

    async with TTSClient('127.0.0.1', 5555) as client:
        for rate in rates:
            try:
                result = await client.synthesize(
                    text=text,
                    speaker="en-US-AriaNeural",
                    language="en-US",
                    rate=rate
                )

                if result.success:
                    speed_label = "slow" if rate < 1.0 else "fast" if rate > 1.0 else "normal"
                    print_success(f"Rate {rate:4.2f}x ({speed_label:6}) - {result.duration_ms}ms")
                else:
                    print_error(f"Rate {rate:4.2f}x - {result.message}")
            except Exception as e:
                print_error(f"Rate {rate:4.2f}x - {e}")


async def test_long_text():
    """Test synthesis with longer text"""
    print_header("Test 5: Long Text")

    long_text = """
    The quick brown fox jumps over the lazy dog. This is a pangram, a sentence that contains
    every letter of the alphabet at least once. Pangrams are useful for testing fonts and
    keyboards. The most well-known English pangram is "The quick brown fox jumps over the
    lazy dog." Another example is "Pack my box with five dozen liquor jugs."
    """

    try:
        async with TTSClient('127.0.0.1', 5555) as client:
            result = await client.synthesize(
                text=long_text,
                speaker="en-US-AriaNeural",
                language="en-US"
            )

            if result.success:
                print_success("Long text synthesis completed")
                print(f"  Text length: {len(long_text)} characters")
                print(f"  Audio size: {len(result.audio_data)} bytes")
                print(f"  Duration: {result.duration_ms}ms")
                #print(f"  Words: {len(result.boundaries)}")
                return True
            else:
                print_error(f"Failed: {result.message}")
                return False
    except Exception as e:
        print_error(f"Error: {e}")
        return False


async def test_error_handling():
    """Test error handling"""
    print_header("Test 7: Error Handling")

    test_cases = [
        ({"text": "", "speaker": "en-US-AriaNeural", "language": "en-US"}, "Empty text"),
        ({"text": "Hello", "speaker": "invalid-voice", "language": "en-US"}, "Invalid speaker"),
        ({"text": "Hello", "language": "en-US"}, "Missing speaker"),
        ({"text": "Hello", "speaker": "en-US-AriaNeural"}, "Missing language"),
        ({"text": "Hello", "speaker": "en-US-AriaNeural", "language": "en-US", "rate": 5.0}, "Invalid rate"),
    ]

    passed = 0
    async with TTSClient('127.0.0.1', 5555) as client:
        for i, (request, description) in enumerate(test_cases, 1):
            try:
                # Use the raw protocol to test error cases
                import json
                import struct

                request_json = json.dumps(request, ensure_ascii=False).encode('utf-8')
                length = struct.pack('!I', len(request_json))

                client.writer.write(length + request_json)
                await client.writer.drain()

                # Read response
                length_data = await client.reader.readexactly(4)
                response_length = struct.unpack('!I', length_data)[0]
                response_data = await client.reader.readexactly(response_length)
                response = json.loads(response_data.decode('utf-8'))

                if response['status'] == 'error':
                    print_success(f"Correctly caught: {description}")
                    print(f"  Message: {response['message']}")
                    passed += 1
                else:
                    print_error(f"Should have failed: {description}")
            except Exception as e:
                print_error(f"Error testing {description}: {e}")

    print(f"\nError handling: {passed}/{len(test_cases)} tests passed")
    return passed == len(test_cases)


async def demo_karaoke():
    """Create karaoke/subtitle timing data"""
    print_header("Demo: Karaoke Timing Data")

    text = "The quick brown fox jumps over the lazy dog."

    try:
        async with TTSClient('127.0.0.1', 5555) as client:
            result = await client.synthesize(
                text=text,
                speaker="en-US-AriaNeural",
                language="en-US"
            )

            if result.success:
                # Create karaoke data structure
                karaoke = {
                    "text": text,
                    "duration_ms": result.duration_ms,
                    "cues": [
                        {
                            "start_ms": b.offset,
                            "text": b.text
                        }
                        for b in result.boundaries
                    ]
                }

                print_success("Karaoke data created")
                print("\nJSON output:")
                print(json.dumps(karaoke, indent=2))
                return True
            else:
                print_error(f"Failed: {result.message}")
                return False
    except Exception as e:
        print_error(f"Error: {e}")
        return False


async def run_all_tests():
    """Run all tests"""
    print(f"\n{Colors.BOLD}{'='*60}")
    print(f"Edge TTS Bridge Server - Test Suite")
    print(f"{'='*60}{Colors.RESET}\n")

    tests = [
        ("Basic Synthesis", test_basic_synthesis),
        ("Word Boundaries", test_word_boundaries),
        ("Multiple Voices", test_multiple_voices),
        ("Speech Rates", test_speech_rates),
        ("Long Text", test_long_text),
        ("Error Handling", test_error_handling),
        ("Karaoke Demo", demo_karaoke),
    ]

    results = {}
    for name, test_func in tests:
        try:
            results[name] = await test_func()
        except Exception as e:
            print_error(f"Test failed with exception: {e}")
            results[name] = False

    # Summary
    print_header("Test Summary")
    passed = sum(1 for v in results.values() if v)
    total = len(results)

    for name, passed_flag in results.items():
        status = f"{Colors.GREEN}PASS{Colors.RESET}" if passed_flag else f"{Colors.RED}FAIL{Colors.RESET}"
        print(f"  {status} {name}")

    print(f"\n{Colors.BOLD}Total: {passed}/{total} tests passed{Colors.RESET}\n")

    return passed == total


def main():
    """Main entry point"""
    if len(sys.argv) > 1:
        # Run specific test
        test_name = sys.argv[1]
        test_map = {
            'basic': test_basic_synthesis,
            'boundaries': test_word_boundaries,
            'voices': test_multiple_voices,
            'rates': test_speech_rates,
            'long': test_long_text,
            'errors': test_error_handling,
            'karaoke': demo_karaoke,
        }

        if test_name not in test_map:
            print(f"Unknown test: {test_name}")
            print(f"Available: {', '.join(test_map.keys())}")
            sys.exit(1)

        asyncio.run(test_map[test_name]())
    else:
        # Run all tests
        success = asyncio.run(run_all_tests())
        sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
