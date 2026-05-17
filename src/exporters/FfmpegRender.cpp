#include "FfmpegRender.h"

#include <iostream>
#include <sstream>
#include <filesystem>

#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

FfmpegRender::FfmpegRender( std::string filename, std::string codec, double framerate )
	: _filename( std::move( filename ) ), _codec( std::move( codec ) ), _framerate( framerate )
{
}

FfmpegRender::~FfmpegRender()
{
	close();
}

void FfmpegRender::encode( const lunasvg::Bitmap &bitmap )
{
	if ( bitmap.isNull() )
	{
		return;
	}

	uint32_t width = static_cast<uint32_t>( bitmap.width() );
	uint32_t height = static_cast<uint32_t>( bitmap.height() );

	if ( !_pipe )
	{
		open_pipe( width, height );
	}

	if ( _pipe )
	{
		if ( width != _width || height != _height )
		{
			std::cerr << "Warning: Bitmap size changed during encoding. This is not supported by "
						 "the current FFmpeg pipe configuration.\n";
			return;
		}

		// Write frame data row by row to handle potential stride/padding
		uint32_t row_size = _width * 4;
		const uint8_t *data = bitmap.data();
		uint32_t stride = static_cast<uint32_t>( bitmap.stride() );

		for ( uint32_t y = 0; y < _height; ++y )
		{
			fwrite( data + y * stride, 1, row_size, _pipe );
		}
	}
}

void FfmpegRender::open_pipe( uint32_t width, uint32_t height )
{
	_width = width;
	_height = height;

	std::stringstream cmd;
	cmd << "ffmpeg -y -hide_banner -loglevel warning";

	// -f rawvideo tells ffmpeg to expect raw pixel data.
	cmd << " -f rawvideo";

	// We use -pixel_format bgra because LunaSVG (via PlutoVG) typically uses BGRA in memory.
	cmd << " -pixel_format bgra";

	cmd << " -video_size " << width << "x" << height << ' ' << "-framerate " << _framerate;

	// -i - tells ffmpeg to read from stdin (the pipe).
	cmd << " -i -";

	// output -pix_fmt yuv420p is for maximum compatibility with players.
	cmd << " -pix_fmt yuv420p";

	if ( _codec == "h264" || _codec.empty() )
	{
		// High quality AVC / H264
		cmd << " -c:v libx264";
		cmd << " -crf 18";
	}
	else if ( _codec == "h265" )
	{
		// High quality HEVC / H265
		cmd << " -c:v libx265";
		cmd << " -crf 21";
	}

	cmd << ' ' << '"' << _filename << '"';

	std::string cmd_str = cmd.str();

#ifdef _WIN32
	m_pipe = _popen( cmd_str.c_str(), "wb" );
#else
	_pipe = popen( cmd_str.c_str(), "w" );
#endif

	if ( !_pipe )
	{
		std::cerr << "Error: Could not open FFmpeg pipe.\n";
	}
}

void FfmpegRender::close()
{
	if ( _pipe )
	{
		PCLOSE( _pipe );
		_pipe = nullptr;
	}
}

void FfmpegRender::mux( double framerate,
	const std::string &video_file,
	const std::string &audio_file,
	const std::string &output_file )
{
	(void)framerate;
	std::stringstream cmd;
	// -i video_file: input video
	// -i audio_file: input audio
	// -c:v copy: copy video codec (re-encoding not needed)
	// -c:a aac: encode audio as AAC (highly compatible)
	// -map 0:v:0: take first video stream from first input
	// -map 1:a:0: take first audio stream from second input
	// -shortest: stop when the shortest stream ends (usually narration)
	// -y: overwrite output
	cmd << "ffmpeg -y -hide_banner -loglevel warning";
	cmd << " -i " << '"' << video_file << '"';
	cmd << " -i " << '"' << audio_file << '"';
	cmd << " -c:v copy -map 0:v:0";
	cmd << " -c:a aac -map 1:a:0";
	cmd << " -shortest";
	cmd << ' ' << '"' << output_file << '"';

	std::string cmd_str = cmd.str();
	int result = std::system( cmd_str.c_str() );
	if ( result != 0 )
	{
		std::cerr << "Error: FFmpeg muxing failed with code " << result << "\n";
	}
}
