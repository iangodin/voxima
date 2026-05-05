#include "TTSClient.h"
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <kj/io.h>
#include <cstring>
#include <cstddef>
#include <sndfile.h>

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

namespace {

struct VirtualFile {
    const uint8_t* data;
    sf_count_t size;
    sf_count_t offset;
};

sf_count_t get_filelen(void* user_data) {
    return static_cast<VirtualFile*>(user_data)->size;
}

sf_count_t seek(sf_count_t offset, int whence, void* user_data) {
    auto* vf = static_cast<VirtualFile*>(user_data);
    switch (whence) {
        case SEEK_SET: vf->offset = offset; break;
        case SEEK_CUR: vf->offset += offset; break;
        case SEEK_END: vf->offset = vf->size + offset; break;
        default: break;
    }
    return vf->offset;
}

sf_count_t read(void* ptr, sf_count_t count, void* user_data) {
    auto* vf = static_cast<VirtualFile*>(user_data);
    if (vf->offset + count > vf->size) {
        count = vf->size - vf->offset;
    }
    if (count > 0) {
        std::memcpy(ptr, vf->data + vf->offset, static_cast<size_t>(count));
        vf->offset += count;
    }
    return count;
}

sf_count_t write(const void*, sf_count_t, void*) {
    return 0;
}

sf_count_t tell(void* user_data) {
    return static_cast<VirtualFile*>(user_data)->offset;
}

} // namespace

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

        kj::ArrayInputStream inputStream(kj::arrayPtr(respData.data(), respData.size()));
        capnp::InputStreamMessageReader responseMessage(inputStream);
        TTSResponse::Reader response = responseMessage.getRoot<TTSResponse>();

        result.status = response.getStatus().cStr();
        result.message = response.getMessage().cStr();
        result.success = (result.status == "success");

        if (response.hasAudio()) {
            auto audioData = response.getAudio();
            VirtualFile vf = { audioData.begin(), static_cast<sf_count_t>(audioData.size()), 0 };
            SF_VIRTUAL_IO vio = { get_filelen, seek, read, write, tell };
            SF_INFO sfinfo = {};
            SNDFILE* sndfile = sf_open_virtual(&vio, SFM_READ, &sfinfo, &vf);

            if (sndfile) {
                AudioBuffer buffer(static_cast<size_t>(sfinfo.frames), static_cast<size_t>(sfinfo.samplerate), static_cast<size_t>(sfinfo.channels));
                sf_readf_float(sndfile, buffer.data(), sfinfo.frames);
                sf_close(sndfile);
                result.audio = std::move(buffer);
            }
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

    } catch (const std::exception& e) {
        result.success = false;
        result.status = "error";
        result.message = e.what();
    }

    return result;
}

} // namespace tts
