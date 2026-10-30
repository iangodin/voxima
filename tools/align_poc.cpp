// Forced-alignment proof of concept: synthesizes a line of narration via
// TTSClient, then runs the new align() pass over the result and prints
// word-level timestamps. Manual test tool for the Qwen3 Forced Aligner
// integration (tts.md phase 4) - mirrors tools/tts_poc.cpp's role for
// synthesis.

#include <tts/TTSClient.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{

std::string read_text_file( const std::string &path )
{
	std::ifstream file( path );
	if ( !file )
		throw std::runtime_error( "Could not open text file: " + path );

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

}

int main( int argc, char *argv[] )
{
	if ( argc < 6 )
	{
		std::cerr << "Usage: " << argv[0]
				  << " <text_file> <chatterbox_gguf> <aligner_gguf> <voice_ref_wav> <lang> [backend=best]\n";
		return 1;
	}

	const std::string text_path = argv[1];
	const std::string model_path = argv[2];
	const std::string aligner_path = argv[3];
	const std::string voice_ref_path = argv[4];
	const std::string language = argv[5];
	const std::string backend = argc > 6 ? argv[6] : "best";

	try
	{
		const std::string text = read_text_file( text_path );

		tts::TTSClient client( model_path, backend, 4, aligner_path );
		client.setup( ".cache/align-poc" );

		std::cout << "Synthesizing " << text.size() << " characters (" << language << ")...\n";
		auto synth = client.synthesize( text, voice_ref_path, language );
		if ( !synth.success || !synth.audio )
		{
			std::cerr << "Error: synthesis failed: " << synth.message << "\n";
			return 1;
		}
		std::cout << "Audio: " << synth.audio->duration() << " s\n";

		std::cout << "Aligning...\n";
		auto words = client.align( text, *synth.audio, language );
		std::cout << "Words: " << words.size() << "\n";
		for ( auto &w: words )
		{
			std::cout << "  [" << std::fixed << std::setprecision( 3 ) << w.start_seconds << " - "
					  << w.end_seconds << "] \"" << w.word << "\" (confidence " << w.confidence << ")\n";
		}
	}
	catch ( const std::exception &e )
	{
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}
