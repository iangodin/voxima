
#pragma once

#include "asio_wrapper.h"

#include <audio/AudioBuffer.h>

#include <filesystem>
#include <string>
#include <vector>
#include <optional>

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
	TTSClient(
		asio::io_context &io_context,
		std::string host,
		uint16_t port,
		std::optional<std::filesystem::path> cache_dir = std::nullopt );

	SynthesisResult synthesize(
		const std::string &text,
		const std::string &speaker,
		const std::string &language,
		float rate = 1.0f );

	void setCacheDirectory( const std::filesystem::path &path );

private:
	asio::io_context &io_context_;
	std::string host_;
	uint16_t port_;
	std::optional<std::filesystem::path> cache_dir_;
};

}

