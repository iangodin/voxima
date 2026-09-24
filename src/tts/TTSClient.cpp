#include "TTSClient.h"

#include <audiocpp.h>

#include <sndfile.h>

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace tts
{

namespace
{

uint64_t fnv1a( const std::string &s )
{
	uint64_t hash = 0xcbf29ce484222325;
	for ( char c: s )
	{
		hash ^= static_cast<uint8_t>( c );
		hash *= 0x100000001b3;
	}
	return hash;
}

// Model filename is folded into the key so swapping the GGUF (a quantization
// change, a version bump) automatically invalidates old cache entries
// instead of silently serving audio from a different model.
std::string compute_hash(
	const std::string &model_name,
	const std::string &text,
	const std::filesystem::path &voice_ref_path,
	const std::string &language,
	float rate )
{
	std::stringstream ss;
	ss << model_name << "|" << voice_ref_path.string() << "|" << language << "|"
	   << std::fixed << std::setprecision( 2 ) << rate << "|" << text;
	uint64_t h = fnv1a( ss.str() );
	std::stringstream res;
	res << std::hex << std::setw( 16 ) << std::setfill( '0' ) << h;
	return res.str();
}

std::vector<float> read_reference_clip( const std::filesystem::path &path, int &sample_rate, int &channels )
{
	SF_INFO sfinfo = {};
	SNDFILE *sndfile = sf_open( path.string().c_str(), SFM_READ, &sfinfo );
	if ( !sndfile )
	{
		throw std::runtime_error( "TTS: could not open reference clip: " + path.string() );
	}

	std::vector<float> samples( static_cast<size_t>( sfinfo.frames ) * sfinfo.channels );
	sf_readf_float( sndfile, samples.data(), sfinfo.frames );
	sf_close( sndfile );

	sample_rate = sfinfo.samplerate;
	channels = sfinfo.channels;
	return samples;
}

std::optional<AudioBuffer> read_wav( const std::filesystem::path &path )
{
	SF_INFO sfinfo = {};
	SNDFILE *sndfile = sf_open( path.string().c_str(), SFM_READ, &sfinfo );
	if ( !sndfile )
	{
		return std::nullopt;
	}

	AudioBuffer buffer(
		static_cast<size_t>( sfinfo.frames ),
		static_cast<size_t>( sfinfo.samplerate ),
		static_cast<size_t>( sfinfo.channels ) );
	sf_readf_float( sndfile, buffer.data(), sfinfo.frames );
	sf_close( sndfile );
	return buffer;
}

void write_wav( const std::filesystem::path &path, const AudioBuffer &buffer )
{
	SF_INFO sfinfo = {};
	sfinfo.channels = static_cast<int>( buffer.channels() );
	sfinfo.samplerate = static_cast<int>( buffer.rate() );
	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	SNDFILE *sndfile = sf_open( path.string().c_str(), SFM_WRITE, &sfinfo );
	if ( !sndfile )
	{
		return;
	}
	sf_writef_float( sndfile, buffer.data(), static_cast<sf_count_t>( buffer.frames() ) );
	sf_close( sndfile );
}

}

struct RegistryDeleter
{
	void operator()( audiocpp_registry *r ) const
	{
		audiocpp_registry_free( r );
	}
};
struct ModelDeleter
{
	void operator()( audiocpp_model *m ) const
	{
		audiocpp_model_free( m );
	}
};
struct SessionDeleter
{
	void operator()( audiocpp_session *s ) const
	{
		audiocpp_session_free( s );
	}
};

struct RegistryHandle
{
	std::unique_ptr<audiocpp_registry, RegistryDeleter> ptr;
};
struct ModelHandle
{
	std::unique_ptr<audiocpp_model, ModelDeleter> ptr;
};
struct SessionHandle
{
	std::unique_ptr<audiocpp_session, SessionDeleter> ptr;
};

TTSClient::TTSClient(
	std::filesystem::path model_path,
	std::string backend,
	int threads,
	std::filesystem::path aligner_model_path ) :
	_model_path( std::move( model_path ) ),
	_backend( backend ),
	_threads( threads ),
	_aligner_model_path( std::move( aligner_model_path ) )
{
	audiocpp_registry *registry_raw = nullptr;
	if ( audiocpp_registry_create( nullptr, &registry_raw ) != AUDIOCPP_OK )
	{
		throw std::runtime_error( std::string( "TTS: registry: " ) + audiocpp_last_error() );
	}
	_registry = std::make_unique<RegistryHandle>( RegistryHandle{
		std::unique_ptr<audiocpp_registry, RegistryDeleter>( registry_raw ) } );

	audiocpp_model_config load_config = {};
	load_config.family_hint = "chatterbox";

	audiocpp_model *model_raw = nullptr;
	if ( audiocpp_model_load(
			 _registry->ptr.get(), _model_path.string().c_str(), &load_config, nullptr, &model_raw ) !=
		 AUDIOCPP_OK )
	{
		throw std::runtime_error(
			"TTS: failed to load model " + _model_path.string() + ": " + audiocpp_last_error() );
	}
	_model = std::make_unique<ModelHandle>( ModelHandle{
		std::unique_ptr<audiocpp_model, ModelDeleter>( model_raw ) } );

	// "clon" is audio.cpp's internal task token for voice cloning - Chatterbox
	// always needs a reference clip, there is no built-in-voice TTS path for
	// this family (only for the separate chatterbox_turbo family).
	audiocpp_backend_config backend_config = {};
	backend_config.backend = backend.c_str();
	backend_config.device = 0;
	backend_config.threads = threads;

	audiocpp_session *session_raw = nullptr;
	if ( audiocpp_session_create( _model->ptr.get(), "clon", "offline", &backend_config, nullptr, &session_raw ) !=
		 AUDIOCPP_OK )
	{
		throw std::runtime_error( std::string( "TTS: session: " ) + audiocpp_last_error() );
	}
	_session = std::make_unique<SessionHandle>( SessionHandle{
		std::unique_ptr<audiocpp_session, SessionDeleter>( session_raw ) } );
}

TTSClient::~TTSClient() = default;
TTSClient::TTSClient( TTSClient && ) noexcept = default;
TTSClient &TTSClient::operator=( TTSClient && ) noexcept = default;

void TTSClient::setup( const std::filesystem::path &render_dir )
{
	_cache_dir = render_dir / "tts-cache";
}

SynthesisResult TTSClient::synthesize(
	const std::string &text,
	const std::filesystem::path &voice_ref_path,
	const std::string &language,
	float rate )
{
	SynthesisResult result;
	result.success = false;

	const std::string model_name = _model_path.filename().string();

	std::optional<std::filesystem::path> cache_path;
	if ( !_cache_dir.empty() )
	{
		try
		{
			if ( !std::filesystem::exists( _cache_dir ) )
			{
				std::filesystem::create_directories( _cache_dir );
			}
			cache_path = _cache_dir /
				( compute_hash( model_name, text, voice_ref_path, language, rate ) + ".wav" );

			if ( std::filesystem::exists( *cache_path ) )
			{
				if ( auto cached = read_wav( *cache_path ) )
				{
					result.success = true;
					result.status = "success";
					result.audio = std::move( cached );
					return result;
				}
			}
		}
		catch ( ... )
		{
			// If cache reading fails, fall through and synthesize.
		}
	}

	try
	{
		int voice_rate = 0;
		int voice_channels = 0;
		std::vector<float> voice_ref = read_reference_clip( voice_ref_path, voice_rate, voice_channels );

		audiocpp_request *request_raw = audiocpp_request_create();
		if ( !request_raw )
		{
			throw std::runtime_error( "TTS: request allocation failed" );
		}
		std::unique_ptr<audiocpp_request, decltype( &audiocpp_request_free )> request(
			request_raw, &audiocpp_request_free );

		if ( audiocpp_request_set_text( request.get(), text.c_str(), language.c_str() ) != AUDIOCPP_OK )
		{
			throw std::runtime_error( std::string( "TTS: set_text: " ) + audiocpp_last_error() );
		}
		if ( audiocpp_request_set_voice_audio(
				 request.get(), voice_ref.data(), voice_ref.size() / voice_channels, voice_rate, voice_channels ) !=
			 AUDIOCPP_OK )
		{
			throw std::runtime_error( std::string( "TTS: set_voice_audio: " ) + audiocpp_last_error() );
		}

		if ( rate != 1.0f && audiocpp_model_supports_style_condition( _model->ptr.get() ) )
		{
			audiocpp_request_set_speaking_rate( request.get(), rate );
		}

		audiocpp_result *result_raw = nullptr;
		if ( audiocpp_session_run( _session->ptr.get(), request.get(), &result_raw ) != AUDIOCPP_OK )
		{
			throw std::runtime_error( std::string( "TTS: run: " ) + audiocpp_last_error() );
		}
		std::unique_ptr<audiocpp_result, decltype( &audiocpp_result_free )> synth_result(
			result_raw, &audiocpp_result_free );

		const float *samples = nullptr;
		size_t frames = 0;
		int out_rate = 0;
		int out_channels = 0;
		if ( audiocpp_result_audio( synth_result.get(), &samples, &frames, &out_rate, &out_channels ) !=
			 AUDIOCPP_OK )
		{
			throw std::runtime_error( std::string( "TTS: result_audio: " ) + audiocpp_last_error() );
		}

		AudioBuffer buffer(
			frames, static_cast<size_t>( out_rate ), static_cast<size_t>( out_channels ) );
		std::copy( samples, samples + frames * static_cast<size_t>( out_channels ), buffer.data() );

		result.success = true;
		result.status = "success";
		result.audio = std::move( buffer );

		if ( cache_path )
		{
			write_wav( *cache_path, result.audio.value() );
		}
	}
	catch ( const std::exception &e )
	{
		result.success = false;
		result.status = "error";
		result.message = e.what();
		std::cerr << "TTS synthesis failed: " << e.what() << std::endl;
	}

	return result;
}

std::vector<WordTimestamp> TTSClient::align(
	const std::string &text,
	const AudioBuffer &audio,
	const std::string &language )
{
	if ( _aligner_model_path.empty() )
	{
		throw std::runtime_error( "TTS: align() called but no aligner model path was configured" );
	}

	if ( !_aligner_session )
	{
		audiocpp_model_config load_config = {};
		load_config.family_hint = "qwen3_forced_aligner";

		audiocpp_model *model_raw = nullptr;
		if ( audiocpp_model_load(
				 _registry->ptr.get(), _aligner_model_path.string().c_str(), &load_config, nullptr, &model_raw ) !=
			 AUDIOCPP_OK )
		{
			throw std::runtime_error(
				"TTS: failed to load aligner model " + _aligner_model_path.string() + ": " +
				audiocpp_last_error() );
		}
		_aligner_model = std::make_unique<ModelHandle>( ModelHandle{
			std::unique_ptr<audiocpp_model, ModelDeleter>( model_raw ) } );

		audiocpp_backend_config backend_config = {};
		backend_config.backend = _backend.c_str();
		backend_config.device = 0;
		backend_config.threads = _threads;

		audiocpp_session *session_raw = nullptr;
		if ( audiocpp_session_create(
				 _aligner_model->ptr.get(), "align", "offline", &backend_config, nullptr, &session_raw ) !=
			 AUDIOCPP_OK )
		{
			throw std::runtime_error( std::string( "TTS: aligner session: " ) + audiocpp_last_error() );
		}
		_aligner_session = std::make_unique<SessionHandle>( SessionHandle{
			std::unique_ptr<audiocpp_session, SessionDeleter>( session_raw ) } );
	}

	audiocpp_request *request_raw = audiocpp_request_create();
	if ( !request_raw )
	{
		throw std::runtime_error( "TTS: aligner request allocation failed" );
	}
	std::unique_ptr<audiocpp_request, decltype( &audiocpp_request_free )> request(
		request_raw, &audiocpp_request_free );

	if ( audiocpp_request_set_text( request.get(), text.c_str(), language.c_str() ) != AUDIOCPP_OK )
	{
		throw std::runtime_error( std::string( "TTS: aligner set_text: " ) + audiocpp_last_error() );
	}
	if ( audiocpp_request_set_audio(
			 request.get(),
			 audio.data(),
			 audio.frames(),
			 static_cast<int>( audio.rate() ),
			 static_cast<int>( audio.channels() ) ) != AUDIOCPP_OK )
	{
		throw std::runtime_error( std::string( "TTS: aligner set_audio: " ) + audiocpp_last_error() );
	}

	audiocpp_result *result_raw = nullptr;
	if ( audiocpp_session_run( _aligner_session->ptr.get(), request.get(), &result_raw ) != AUDIOCPP_OK )
	{
		throw std::runtime_error( std::string( "TTS: aligner run: " ) + audiocpp_last_error() );
	}
	std::unique_ptr<audiocpp_result, decltype( &audiocpp_result_free )> align_result(
		result_raw, &audiocpp_result_free );

	// Qwen3 Forced Aligner reports start/end in samples at its own internal
	// frontend rate (a Whisper-style 16 kHz audio frontend - see
	// extern/audio.cpp/src/models/qwen3_asr/frontend_whisper.cpp, which the
	// aligner shares the frontend design with), not the input audio's rate.
	// audiocpp_request_set_audio() is given the real input rate and resamples
	// internally, so this is a fixed model constant, not something we choose.
	constexpr double kAlignerSampleRate = 16000.0;

	std::vector<WordTimestamp> words;
	size_t word_count = audiocpp_result_word_count( align_result.get() );
	words.reserve( word_count );
	for ( size_t i = 0; i < word_count; ++i )
	{
		const char *word_text = nullptr;
		int64_t start_sample = 0;
		int64_t end_sample = 0;
		float confidence = 0.0f;
		if ( audiocpp_result_word( align_result.get(), i, &word_text, &start_sample, &end_sample, &confidence ) !=
			 AUDIOCPP_OK )
		{
			continue;
		}
		words.push_back( WordTimestamp{
			word_text ? word_text : "",
			static_cast<double>( start_sample ) / kAlignerSampleRate,
			static_cast<double>( end_sample ) / kAlignerSampleRate,
			confidence } );
	}

	return words;
}

}
