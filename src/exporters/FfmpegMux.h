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

	// is_default marks the track with the ffmpeg "default" disposition, so
	// players pick it automatically - intended for the "master" (first)
	// --lang track. Non-default tracks get their disposition explicitly
	// cleared rather than left to ffmpeg's own heuristics.
	void add_audio_track(
		std::string_view audio, std::string_view lang, std::string_view title, bool is_default = false );
	void add_subtitle_track(
		std::string_view subtitle, std::string_view lang, std::string_view title, bool is_default = false );

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

