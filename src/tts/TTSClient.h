
#pragma once

#include <audio/AudioBuffer.h>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tts
{

struct SynthesisResult
{
	bool success;
	std::string status;
	std::string message;
	std::optional<AudioBuffer> audio;
};

// A single aligned word from align(), grounded in the actual synthesized
// audio via forced alignment (as opposed to edge-tts's old streamed
// WordBoundary estimates, which this replaces conceptually).
struct WordTimestamp
{
	std::string word;
	double start_seconds;
	double end_seconds;
	float confidence;
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
	//
	// aligner_model_path is optional (empty = alignment unavailable). Unlike
	// the Chatterbox model, the aligner is loaded lazily on the first align()
	// call rather than eagerly here - most renders don't need subtitle
	// timing, so they shouldn't pay for a second model load they never use.
	TTSClient(
		std::filesystem::path model_path,
		std::string backend = "best",
		int threads = 4,
		std::filesystem::path aligner_model_path = {} );
	~TTSClient();

	TTSClient( TTSClient && ) noexcept;
	TTSClient &operator=( TTSClient && ) noexcept;
	TTSClient( const TTSClient & ) = delete;
	TTSClient &operator=( const TTSClient & ) = delete;

	// Where speech is kept once made, to be used again when the same words are wanted in the same voice
	void setup( const std::filesystem::path &cache_dir );

	// Whether an aligner_model_path was configured at construction. Check
	// this before calling align() for languages/renders where subtitle
	// timing wasn't requested, rather than relying on align()'s throw for
	// the (common, expected) "no aligner configured" case.
	bool has_aligner() const
	{
		return !_aligner_model_path.empty();
	}

	// voice_ref_path is a reference clip to clone the speaker from. rate is
	// a speaking-rate multiplier (1.0 = unchanged); only forwarded to the
	// model if the loaded family advertises style-conditioning support.
	SynthesisResult synthesize(
		const std::string &text,
		const std::filesystem::path &voice_ref_path,
		const std::string &language,
		float rate = 1.0f );

	// Forced-alignment pass over already-synthesized audio: returns
	// word-level timestamps grounded in the actual waveform. Requires
	// aligner_model_path to have been set at construction; throws otherwise.
	// Loads the aligner model on first use.
	std::vector<WordTimestamp> align(
		const std::string &text,
		const AudioBuffer &audio,
		const std::string &language );

private:
	std::filesystem::path _model_path;
	std::string _backend;
	int _threads;
	std::unique_ptr<RegistryHandle> _registry;
	std::unique_ptr<ModelHandle> _model;
	std::unique_ptr<SessionHandle> _session;
	std::filesystem::path _cache_dir;

	std::filesystem::path _aligner_model_path;
	std::unique_ptr<ModelHandle> _aligner_model;
	std::unique_ptr<SessionHandle> _aligner_session;
};

}
