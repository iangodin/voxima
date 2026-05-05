#pragma once

#include <string>
#include <vector>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
#include <asio.hpp>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "protocol.capnp.h"

namespace tts {

struct WordBoundary {
    std::string text;
    uint64_t offset;
    uint64_t duration;
};

struct SynthesisResult {
    bool success;
    std::string status;
    std::string message;
    std::vector<std::byte> audio_data;
    std::vector<WordBoundary> boundaries;
    uint64_t duration_ms;
    uint32_t sample_rate;
    uint8_t channels;
    std::string format;
};

class TTSClient {
public:
    TTSClient(asio::io_context& io_context, std::string host, uint16_t port);

    SynthesisResult synthesize(
        const std::string& text,
        const std::string& speaker,
        const std::string& language,
        float rate = 1.0f
    );

private:
    asio::io_context& io_context_;
    std::string host_;
    uint16_t port_;
};

} // namespace tts
