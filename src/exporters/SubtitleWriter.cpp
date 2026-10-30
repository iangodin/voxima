#include "SubtitleWriter.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace
{

std::string format_timestamp( double seconds, char ms_separator )
{
	if ( seconds < 0.0 )
	{
		seconds = 0.0;
	}

	auto total_ms = static_cast<long long>( seconds * 1000.0 + 0.5 );
	long long ms = total_ms % 1000;
	long long total_s = total_ms / 1000;
	long long s = total_s % 60;
	long long total_m = total_s / 60;
	long long m = total_m % 60;
	long long h = total_m / 60;

	std::ostringstream out;
	out << std::setfill( '0' ) << std::setw( 2 ) << h << ':' << std::setw( 2 ) << m << ':'
		<< std::setw( 2 ) << s << ms_separator << std::setw( 3 ) << ms;
	return out.str();
}

}

void write_srt( const std::string &path, const std::vector<SubtitleCue> &cues )
{
	std::ofstream out( path );
	if ( !out )
	{
		throw std::runtime_error( "unable to open subtitle file " + path );
	}

	size_t index = 1;
	for ( const auto &cue: cues )
	{
		out << index++ << '\n';
		out << format_timestamp( cue.start_seconds, ',' ) << " --> "
			<< format_timestamp( cue.end_seconds, ',' ) << '\n';
		out << cue.text << "\n\n";
	}
}

void write_vtt( const std::string &path, const std::vector<SubtitleCue> &cues )
{
	std::ofstream out( path );
	if ( !out )
	{
		throw std::runtime_error( "unable to open subtitle file " + path );
	}

	out << "WEBVTT\n\n";

	size_t index = 1;
	for ( const auto &cue: cues )
	{
		out << index++ << '\n';
		out << format_timestamp( cue.start_seconds, '.' ) << " --> "
			<< format_timestamp( cue.end_seconds, '.' ) << '\n';
		out << cue.text << "\n\n";
	}
}
