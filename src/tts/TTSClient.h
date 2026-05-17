
#pragma once

#include "asio_wrapper.h"

#include <audio/AudioBuffer.h>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tts
{

struct WordBoundary
{
	std::string text;
	uint64_t offset;
	uint64_t duration;
};

struct SynthesisResult
{
	bool success;
	std::string status;
	std::string message;
	std::optional<AudioBuffer> audio;
	std::vector<WordBoundary> boundaries;
};

class TTSClient
{
public:
	TTSClient( asio::io_context &io_context, std::string host, uint16_t port );

	void setup( const std::filesystem::path &render_dir );

	SynthesisResult synthesize(
		const std::string &text,
		const std::string &speaker,
		const std::string &language,
		float rate = 1.0f );

private:
	asio::io_context &_io_context;
	std::string _host;
	uint16_t _port;
	std::filesystem::path _cache_dir;
};

}
