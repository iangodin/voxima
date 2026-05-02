#pragma once

#include <string>
#include <cstdio>
#include <cstdint>
#include <lunasvg/lunasvg.h>

class FfmpegRender
{
public:
	FfmpegRender( const std::string &filename, double framerate = 30.0 );
	~FfmpegRender();

	void encode( const lunasvg::Bitmap &bitmap );
	void close();

private:
	std::string m_filename;
	double m_framerate;
	FILE *m_pipe = nullptr;
	uint32_t m_width = 0;
	uint32_t m_height = 0;

	void open_pipe( uint32_t width, uint32_t height );
};
