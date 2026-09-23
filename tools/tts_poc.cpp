
// audio.cpp / Chatterbox proof of concept: reads a plain text file, clones a
// reference voice, and writes the synthesized English speech to a WAV file.
// CPU only. See tts.md for the plan this is validating.

#include <audiocpp.h>

#include <sndfile.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace
{

struct RegistryDeleter { void operator()( audiocpp_registry *r ) const { audiocpp_registry_free( r ); } };
struct ModelDeleter { void operator()( audiocpp_model *m ) const { audiocpp_model_free( m ); } };
struct SessionDeleter { void operator()( audiocpp_session *s ) const { audiocpp_session_free( s ); } };
struct RequestDeleter { void operator()( audiocpp_request *r ) const { audiocpp_request_free( r ); } };
struct ResultDeleter { void operator()( audiocpp_result *r ) const { audiocpp_result_free( r ); } };

std::string read_text_file( const std::string &path )
{
	std::ifstream file( path );
	if ( !file )
		throw std::runtime_error( "Could not open text file: " + path );

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

std::vector<float> read_reference_clip( const std::string &path, int &sample_rate, int &channels )
{
	SF_INFO sfinfo = {};
	SNDFILE *sndfile = sf_open( path.c_str(), SFM_READ, &sfinfo );
	if ( !sndfile )
		throw std::runtime_error( "Could not open reference clip: " + path );

	std::vector<float> samples( static_cast<size_t>( sfinfo.frames ) * sfinfo.channels );
	sf_readf_float( sndfile, samples.data(), sfinfo.frames );
	sf_close( sndfile );

	sample_rate = sfinfo.samplerate;
	channels = sfinfo.channels;
	return samples;
}

void write_wav( const std::string &path, const float *samples, size_t frames, int sample_rate, int channels )
{
	SF_INFO sfinfo = {};
	sfinfo.channels = channels;
	sfinfo.samplerate = sample_rate;
	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	SNDFILE *sndfile = sf_open( path.c_str(), SFM_WRITE, &sfinfo );
	if ( !sndfile )
		throw std::runtime_error( "Could not open output file: " + path + " (" + sf_strerror( nullptr ) + ")" );

	sf_writef_float( sndfile, samples, static_cast<sf_count_t>( frames ) );
	sf_close( sndfile );
}

} // namespace

int main( int argc, char *argv[] )
{
	if ( argc < 5 )
	{
		std::cerr << "Usage: " << argv[0]
				  << " <text_file> <chatterbox_gguf> <voice_ref_wav> <out_wav>\n";
		return 1;
	}

	const std::string text_path = argv[1];
	const std::string model_path = argv[2];
	const std::string voice_ref_path = argv[3];
	const std::string out_path = argv[4];

	try
	{
		const std::string text = read_text_file( text_path );

		int voice_rate = 0, voice_channels = 0;
		std::vector<float> voice_ref = read_reference_clip( voice_ref_path, voice_rate, voice_channels );

		audiocpp_registry *registry_raw = nullptr;
		if ( audiocpp_registry_create( nullptr, &registry_raw ) != AUDIOCPP_OK )
			throw std::runtime_error( std::string( "registry: " ) + audiocpp_last_error() );
		std::unique_ptr<audiocpp_registry, RegistryDeleter> registry( registry_raw );

		audiocpp_model_config load_config = {};
		load_config.family_hint = "chatterbox";

		const auto load_start = std::chrono::steady_clock::now();

		audiocpp_model *model_raw = nullptr;
		if ( audiocpp_model_load( registry.get(), model_path.c_str(), &load_config, nullptr, &model_raw ) != AUDIOCPP_OK )
			throw std::runtime_error( std::string( "model load: " ) + audiocpp_last_error() );
		std::unique_ptr<audiocpp_model, ModelDeleter> model( model_raw );

		const auto load_end = std::chrono::steady_clock::now();
		const double load_seconds = std::chrono::duration<double>( load_end - load_start ).count();
		std::cout << "Loaded model in " << load_seconds << " s\n";

		// "clon" is audio.cpp's internal task token for voice cloning
		// ("clone" is the model-spec alias) - Chatterbox always needs a
		// reference clip, there is no built-in voice for this family.
		audiocpp_backend_config backend = {};
		backend.backend = "cpu";
		backend.device = 0;
		backend.threads = 4;

		audiocpp_session *session_raw = nullptr;
		if ( audiocpp_session_create( model.get(), "clon", "offline", &backend, nullptr, &session_raw ) != AUDIOCPP_OK )
			throw std::runtime_error( std::string( "session: " ) + audiocpp_last_error() );
		std::unique_ptr<audiocpp_session, SessionDeleter> session( session_raw );

		audiocpp_request *request_raw = audiocpp_request_create();
		if ( !request_raw )
			throw std::runtime_error( "request allocation failed" );
		std::unique_ptr<audiocpp_request, RequestDeleter> request( request_raw );

		if ( audiocpp_request_set_text( request.get(), text.c_str(), "en" ) != AUDIOCPP_OK )
			throw std::runtime_error( std::string( "set_text: " ) + audiocpp_last_error() );
		if ( audiocpp_request_set_voice_audio( request.get(), voice_ref.data(), voice_ref.size() / voice_channels, voice_rate, voice_channels ) != AUDIOCPP_OK )
			throw std::runtime_error( std::string( "set_voice_audio: " ) + audiocpp_last_error() );

		std::cout << "Synthesizing " << text.size() << " characters via Chatterbox (CPU)...\n";

		const auto synth_start = std::chrono::steady_clock::now();

		audiocpp_result *result_raw = nullptr;
		if ( audiocpp_session_run( session.get(), request.get(), &result_raw ) != AUDIOCPP_OK )
			throw std::runtime_error( std::string( "run: " ) + audiocpp_last_error() );
		std::unique_ptr<audiocpp_result, ResultDeleter> result( result_raw );

		const auto synth_end = std::chrono::steady_clock::now();
		const double synth_seconds = std::chrono::duration<double>( synth_end - synth_start ).count();
		std::cout << "Synthesized in " << synth_seconds << " s\n";

		const float *samples = nullptr;
		size_t frames = 0;
		int rate = 0, channels = 0;
		if ( audiocpp_result_audio( result.get(), &samples, &frames, &rate, &channels ) != AUDIOCPP_OK )
			throw std::runtime_error( std::string( "result_audio: " ) + audiocpp_last_error() );

		write_wav( out_path, samples, frames, rate, channels );

		std::cout << "Wrote " << out_path << ": " << frames << " frames, " << rate
				  << " Hz, " << channels << " ch, " << ( double( frames ) / rate )
				  << " s\n";
	}
	catch ( const std::exception &e )
	{
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}
