#include "TTSClient.h"
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <kj/io.h>
#include <cstring>
#include <cstddef>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
#include <asio/ip/tcp.hpp>
#include <asio/write.hpp>
#include <asio/read.hpp>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

namespace tts {

TTSClient::TTSClient(asio::io_context& io_context, std::string host, uint16_t port)
    : io_context_(io_context), host_(std::move(host)), port_(port) {}

SynthesisResult TTSClient::synthesize(
    const std::string& text,
    const std::string& speaker,
    const std::string& language,
    float rate
) {
    SynthesisResult result;
    result.success = false;

    try {
        asio::ip::tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(host_, std::to_string(port_));
        asio::ip::tcp::socket socket(io_context_);
        asio::connect(socket, endpoints);

        // Build request
        capnp::MallocMessageBuilder message;
        TTSRequest::Builder request = message.initRoot<TTSRequest>();
        request.setText(text);
        request.setSpeaker(speaker);
        request.setLanguage(language);
        request.setRate(rate);

        // Serialize request
        kj::VectorOutputStream outputStream;
        capnp::writeMessage(outputStream, message);
        auto requestBytes = outputStream.getArray();

        // Send length (4 bytes, big-endian)
        uint32_t len = static_cast<uint32_t>(requestBytes.size());
        uint8_t lenBytes[4];
        lenBytes[0] = static_cast<uint8_t>( (len >> 24) & 0xFF );
        lenBytes[1] = static_cast<uint8_t>( (len >> 16) & 0xFF );
        lenBytes[2] = static_cast<uint8_t>( (len >> 8) & 0xFF );
        lenBytes[3] = static_cast<uint8_t>( len & 0xFF );

        asio::write(socket, asio::buffer(lenBytes, 4));
        asio::write(socket, asio::buffer(requestBytes.begin(), requestBytes.size()));

        // Read response length
        uint8_t respLenBytes[4];
        asio::read(socket, asio::buffer(respLenBytes, 4));
        uint32_t respLen = (static_cast<uint32_t>(respLenBytes[0]) << 24) |
                           (static_cast<uint32_t>(respLenBytes[1]) << 16) |
                           (static_cast<uint32_t>(respLenBytes[2]) << 8) |
                           static_cast<uint32_t>(respLenBytes[3]);

        // Read response data
        std::vector<uint8_t> respData(respLen);
        asio::read(socket, asio::buffer(respData.data(), respLen));

        // Parse response
        kj::ArrayPtr<const capnp::word> segments(
            reinterpret_cast<const capnp::word*>(respData.data()),
            respLen / sizeof(capnp::word)
        );
        // Note: This assumes the data is aligned and multiple of word size.
        // Cap'n Proto messages are always a multiple of 8 bytes.

        // Actually, FlatArrayMessageReader is easier if it was a flat array.
        // But writeMessage produces a stream with segment table.
        // So we should use InputStreamMessageReader.

        kj::ArrayInputStream inputStream(kj::arrayPtr(respData.data(), respData.size()));
        capnp::InputStreamMessageReader responseMessage(inputStream);
        TTSResponse::Reader response = responseMessage.getRoot<TTSResponse>();

        result.status = response.getStatus().cStr();
        result.message = response.getMessage().cStr();
        result.success = (result.status == "success");

        if (response.hasAudio()) {
            auto audio = response.getAudio();
            result.audio_data.resize(audio.size());
            std::memcpy(result.audio_data.data(), audio.begin(), audio.size());
        }

        if (response.hasBoundaries()) {
            for (auto boundary : response.getBoundaries()) {
                result.boundaries.push_back({
                    boundary.getText().cStr(),
                    boundary.getOffset(),
                    boundary.getDuration()
                });
            }
        }

        result.duration_ms = response.getDuration();

    } catch (const std::exception& e) {
        result.success = false;
        result.status = "error";
        result.message = e.what();
    }

    return result;
}

} // namespace tts
