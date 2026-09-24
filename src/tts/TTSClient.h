
#pragma once

#include <audio/AudioBuffer.h>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace tts
{

struct SynthesisResult
{
	bool success;
	std::string status;
	std::string message;
	std::optional<AudioBuffer> audio;
};

// Loaded registry/model/session are opaque here so callers don't need to see
// audiocpp.h - defined in TTSClient.cpp. unique_ptr<incomplete> requires the
// destructor (and move ops, since declaring the dtor suppresses the implicit
// ones) to be defined out-of-line, where the types are complete.
struct RegistryHandle;
struct ModelHandle;
struct SessionHandle;

class TTSClient
{
public:
	// Loads the Chatterbox model and creates a synthesis session immediately
	// (fail fast: a missing/bad model file throws here, at startup, rather
	// than on the first synthesize() call mid-render).
	TTSClient(
		std::filesystem::path model_path,
		std::string backend = "best",
		int threads = 4 );
	~TTSClient();

	TTSClient( TTSClient && ) noexcept;
	TTSClient &operator=( TTSClient && ) noexcept;
	TTSClient( const TTSClient & ) = delete;
	TTSClient &operator=( const TTSClient & ) = delete;

	void setup( const std::filesystem::path &render_dir );

	// voice_ref_path is a reference clip to clone the speaker from. rate is
	// a speaking-rate multiplier (1.0 = unchanged); only forwarded to the
	// model if the loaded family advertises style-conditioning support.
	SynthesisResult synthesize(
		const std::string &text,
		const std::filesystem::path &voice_ref_path,
		const std::string &language,
		float rate = 1.0f );

private:
	std::filesystem::path _model_path;
	std::unique_ptr<RegistryHandle> _registry;
	std::unique_ptr<ModelHandle> _model;
	std::unique_ptr<SessionHandle> _session;
	std::filesystem::path _cache_dir;
};

}
