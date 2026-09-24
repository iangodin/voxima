
#include "FfmpegMux.h"

#include <iostream>

FfmpegMux::FfmpegMux( std::string output, std::string audio_codec, std::string subtitle_codec ) :
	_output( std::move( output ) ), _acodec( std::move( audio_codec ) ), _scodec( std::move( subtitle_codec ) )
{
	_command << "ffmpeg -y -hide_banner -loglevel warning";
}

FfmpegMux::~FfmpegMux()
{
}

void FfmpegMux::add_video_track( std::string_view video )
{
	if ( video.empty() )
	{
		throw std::runtime_error( "invalid video file '" + std::string( video ) + "'" );
	}
	_command << " -i " << '"' << video << '"';
	_maps << " -map " << _input_files << ":v:0";
	++_input_files;
}

void FfmpegMux::add_audio_track(
	std::string_view audio, std::string_view lang, std::string_view title, bool is_default )
{
	if ( audio.empty() )
	{
		throw std::runtime_error( "invalid audio file '" + std::string( audio ) + "'" );
	}
	if ( lang.empty() )
	{
		throw std::runtime_error( "invalid language '" + std::string( lang ) + "'" );
	}
	_command << " -i " << '"' << audio << '"';
	_maps << " -map " << _input_files << ":a:0";
	_metadata << " -metadata:s:a:" << _audio_tracks << " language=" << lang;
	if ( !title.empty() )
	{
		_metadata << " -metadata:s:a:" << _audio_tracks << " title=" << title;
	}
	_metadata << " -disposition:a:" << _audio_tracks << ( is_default ? " default" : " 0" );
	++_input_files;
	++_audio_tracks;
}

void FfmpegMux::add_subtitle_track(
	std::string_view subtitle, std::string_view lang, std::string_view title, bool is_default )
{
	if ( subtitle.empty() )
	{
		throw std::runtime_error( "invalid subtitle file '" + std::string( subtitle ) + "'" );
	}
	if ( lang.empty() )
	{
		throw std::runtime_error( "invalid language '" + std::string( lang ) + "'" );
	}
	_command << " -i " << '"' << subtitle << '"';
	_maps << " -map " << _input_files << ":s:0";
	_metadata << " -metadata:s:s:" << _subtitle_tracks << " language=" << lang;
	if ( !title.empty() )
	{
		_metadata << " -metadata:s:s:" << _subtitle_tracks << " title=" << title;
	}
	_metadata << " -disposition:s:" << _subtitle_tracks << ( is_default ? " default" : " 0" );
	++_input_files;
	++_subtitle_tracks;
}

void FfmpegMux::mux()
{
	std::stringstream output;
	output << " -shortest";
	output << " -c:v copy";
	if ( _acodec == "opus" )
	{
		output << " -c:a libopus";
		output << " -b:a 128k";
		output << " -vbr on";
		output << " -compression_level 10";
	}
	else if ( _acodec == "aac" )
	{
		output << " -c:a aac";
		output << " -q:a 2";
	}
	else if ( _acodec == "vorbis" )
	{
		output << " -c:a libvorbis";
		output << " -q:a 5";
	}
	else if ( _acodec == "flac" )
	{
		output << " -c:a flac";
		output << " -compression_level 8";
	}
	else if ( _acodec == "wav" )
	{
		output << " -c:a pcm_s16le";
	}
	else
	{
		throw std::runtime_error( "unknown audio codec for FFmpeg muxing " + _acodec );
	}

	if ( _subtitle_tracks > 0 )
	{
		if ( _scodec.empty() )
		{
			throw std::runtime_error( "subtitle tracks were added but no subtitle codec was set" );
		}
		output << " -c:s " << _scodec;
	}

	output << ' ' << '"' << _output << '"';
	std::string cmd_str = _command.str() + _maps.str() + _metadata.str() + output.str();
	_command.str( {} );
	_maps.str( {} );
	_metadata.str( {} );

	std::cout << "FFmpeg mux command:\n" << cmd_str << '\n';

	int result = std::system( cmd_str.c_str() );
	if ( result != 0 )
	{
		throw std::runtime_error( "FFmpeg muxing failed with code " + std::to_string( result ) );
	}
}

