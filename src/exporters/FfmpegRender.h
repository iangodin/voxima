#pragma once

#include <string>
#include <cstdio>
#include <cstdint>

namespace lunasvg
{
class Bitmap;
}

class FfmpegRender
{
public:
	FfmpegRender( std::string output_filename, std::string codec, double framerate );
	~FfmpegRender();

	void encode( const lunasvg::Bitmap &bitmap );
	void close();

	static void mux( double framerate, const std::string &video_file, const std::string &audio_file, const std::string &output_file );

	static bool supported_codec( const std::string &codec );
	static bool supported_format( const std::string &filename );

private:
	std::string _filename;
	std::string _codec;
	double _framerate;
	FILE *_pipe = nullptr;
	uint32_t _width = 0;
	uint32_t _height = 0;

	void open_pipe( uint32_t width, uint32_t height );
};
