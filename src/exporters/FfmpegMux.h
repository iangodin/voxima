#pragma once

#include <string>
#include <cstdio>
#include <sstream>

class FfmpegMux
{
public:
	// subtitle_codec may be left empty if no subtitle tracks will be added.
	FfmpegMux( std::string output_filename, std::string audio_codec, std::string subtitle_codec = {} );
	~FfmpegMux();

	void add_video_track( std::string_view video );
	void add_audio_track( std::string_view audio, std::string_view lang, std::string_view title );
	void add_subtitle_track( std::string_view subtitle, std::string_view lang, std::string_view title );

	void mux();

private:
	std::stringstream _command;
	std::stringstream _maps;
	std::stringstream _metadata;
	std::string _output;
	std::string _acodec;
	std::string _scodec;
	size_t _input_files = 0;
	size_t _audio_tracks = 0;
	size_t _subtitle_tracks = 0;
};

