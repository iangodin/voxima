#include "FfmpegRender.h"
#include <iostream>
#include <sstream>

#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

FfmpegRender::FfmpegRender( const std::string &filename, double framerate )
	: m_filename( filename ), m_framerate( framerate )
{
}

FfmpegRender::~FfmpegRender()
{
	close();
}

void FfmpegRender::encode( const lunasvg::Bitmap &bitmap )
{
	if ( bitmap.isNull() )
		return;

	uint32_t width = static_cast<uint32_t>( bitmap.width() );
	uint32_t height = static_cast<uint32_t>( bitmap.height() );

	if ( !m_pipe )
	{
		open_pipe( width, height );
	}

	if ( m_pipe )
	{
		if ( width != m_width || height != m_height )
		{
			std::cerr << "Warning: Bitmap size changed during encoding. This is not supported by the current FFmpeg pipe configuration.\n";
			return;
		}

		// Write frame data row by row to handle potential stride/padding
		uint32_t row_size = m_width * 4;
		const uint8_t *data = bitmap.data();
		uint32_t stride = static_cast<uint32_t>( bitmap.stride() );

		for ( uint32_t y = 0; y < m_height; ++y )
		{
			fwrite( data + y * stride, 1, row_size, m_pipe );
		}
	}
}

void FfmpegRender::open_pipe( uint32_t width, uint32_t height )
{
	m_width = width;
	m_height = height;

	std::stringstream cmd;
	// We use -pixel_format bgra because LunaSVG (via PlutoVG) typically uses BGRA in memory on little-endian systems.
	// -f rawvideo tells ffmpeg to expect raw pixel data.
	// -i - tells ffmpeg to read from stdin (the pipe).
	// -pix_fmt yuv420p is for maximum compatibility with players.
	cmd << "ffmpeg -y -f rawvideo -pixel_format bgra -video_size " << width << "x" << height
		<< " -framerate " << m_framerate << " -i - -c:v libx264 -pix_fmt yuv420p -crf 18 \"" << m_filename << "\"";

	std::string cmd_str = cmd.str();

#ifdef _WIN32
	m_pipe = _popen( cmd_str.c_str(), "wb" );
#else
	m_pipe = popen( cmd_str.c_str(), "w" );
#endif

	if ( !m_pipe )
	{
		std::cerr << "Error: Could not open FFmpeg pipe.\n";
	}
}

void FfmpegRender::close()
{
	if ( m_pipe )
	{
		PCLOSE( m_pipe );
		m_pipe = nullptr;
	}
}
