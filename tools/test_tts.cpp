
#include <tts/TTSClient.h>
#include <tts/protocol.capnp.h>

#include <sndfile.h>

#include <iostream>
#include <fstream>

int main( int argc, char *argv[] )
{
	if ( argc < 2 )
	{
		std::cerr << "Usage: " << argv[0] << " <text> [speaker] [language]\n";
		return 1;
	}

	std::string text = argv[1];
	std::string speaker = ( argc > 2 ) ? argv[2] : "en-US-AriaNeural";
	std::string language = ( argc > 3 ) ? argv[3] : "en-US";

	asio::io_context io_context;
	tts::TTSClient client( io_context, "127.0.0.1", 5555, ".cache/tts" );

	std::cout << "Synthesizing: \"" << text << "\" using " << speaker << "...\n";
	auto result = client.synthesize( text, speaker, language );

	if ( result.success )
	{
		std::cout << "✓ Success!\n";
		std::cout << "Status: " << result.status << "\n";
		std::cout << "Message: " << result.message << "\n";

		if ( result.audio )
		{
			std::cout << "Audio decoded:\n";
			std::cout << "  Sample rate: " << result.audio->rate() << " Hz\n";
			std::cout << "  Channels: " << result.audio->channels() << "\n";
			std::cout << "  Frames: " << result.audio->frames() << "\n";
			std::cout << "  Duration: "
					  << ( result.audio->frames() * 1000 / result.audio->rate() )
					  << " ms\n";

			SF_INFO sfinfo = {};
			sfinfo.channels = static_cast<int>( result.audio->channels() );
			sfinfo.samplerate = static_cast<int>( result.audio->rate() );
			sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

			SNDFILE *sndfile = sf_open( "output.wav", SFM_WRITE, &sfinfo );
			if ( sndfile )
			{
				sf_writef_float(
					sndfile,
					result.audio->data(),
					static_cast<sf_count_t>( result.audio->frames() ) );
				sf_close( sndfile );
				std::cout << "Audio saved to output.wav (PCM 16-bit WAV)\n";
			}
			else
			{
				std::cerr << "✗ Error saving WAV file: " << sf_strerror( nullptr )
						  << "\n";
			}
		}
		else
		{
			std::cout << "No audio data received.\n";
		}

		std::cout << "Boundaries: " << result.boundaries.size() << "\n";
		for ( const auto &b: result.boundaries )
		{
			std::cout << "  " << b.offset << "ms: " << b.text << " (" << b.duration
					  << "ms)\n";
		}
	}
	else
	{
		std::cerr << "✗ Error: " << result.message << " (Status: " << result.status
				  << ")\n";
		return 1;
	}

	return 0;
}
