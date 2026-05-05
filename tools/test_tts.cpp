#include <iostream>
#include <fstream>
#include "tts/TTSClient.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <text> [speaker] [language]\n";
        return 1;
    }

    std::string text = argv[1];
    std::string speaker = (argc > 2) ? argv[2] : "en-US-AriaNeural";
    std::string language = (argc > 3) ? argv[3] : "en-US";

    asio::io_context io_context;
    tts::TTSClient client(io_context, "127.0.0.1", 5555);

    std::cout << "Synthesizing: \"" << text << "\" using " << speaker << "...\n";
    auto result = client.synthesize(text, speaker, language);

    if (result.success) {
        std::cout << "✓ Success!\n";
        std::cout << "Status: " << result.status << "\n";
        std::cout << "Message: " << result.message << "\n";
        std::cout << "Audio size: " << result.audio_data.size() << " bytes\n";
        std::cout << "Duration: " << result.duration_ms << " ms\n";
        std::cout << "Boundaries: " << result.boundaries.size() << "\n";

        for (const auto& b : result.boundaries) {
            std::cout << "  " << b.offset << "ms: " << b.text << " (" << b.duration << "ms)\n";
        }

        std::ofstream audio_file("output.wav", std::ios::binary);
        audio_file.write(reinterpret_cast<const char*>(result.audio_data.data()), static_cast<std::streamsize>(result.audio_data.size()));
        std::cout << "Audio saved to output.wav\n";
    } else {
        std::cerr << "✗ Error: " << result.message << " (Status: " << result.status << ")\n";
        return 1;
    }

    return 0;
}
