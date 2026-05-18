#pragma once

#include <string>
#include <cstdio>
#include <sstream>

class FfmpegMux
{
public:
	FfmpegMux( std::string output_filename, std::string audio_codec );
	~FfmpegMux();

	void add_video_track( std::string_view video );
	void add_audio_track( std::string_view audio, std::string_view lang, std::string_view title );

	void mux();

private:
	std::stringstream _command;
	std::stringstream _maps;
	std::stringstream _metadata;
	std::string _output;
	std::string _acodec;
	size_t _input_files = 0;
	size_t _audio_tracks = 0;
};

