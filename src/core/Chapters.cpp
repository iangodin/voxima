#include "Chapters.h"

#include <algorithm>
#include <cmath>
#include <sstream>

std::vector<Chapter> choose_chapters( const std::vector<PlayedScene> &scenes, int64_t total_frames )
{
	const auto has_length = [&]( const PlayedScene &scene ) { return ( scene.end < 0 ? total_frames : scene.end ) > scene.start; };

	std::vector<const PlayedScene *> chosen;
	bool marked = false;
	for ( const PlayedScene &scene: scenes )
	{
		if ( !scene.chapter.empty() && has_length( scene ) )
		{
			chosen.push_back( &scene );
			marked = true;
		}
	}
	if ( !marked )
	{
		// Down from the outermost, until there is more than one
		int deepest = 0;
		for ( const PlayedScene &scene: scenes )
		{
			deepest = std::max( deepest, scene.depth );
		}
		for ( int depth = 0; depth <= deepest && chosen.size() < 2; ++depth )
		{
			chosen.clear();
			for ( const PlayedScene &scene: scenes )
			{
				if ( scene.depth == depth && has_length( scene ) )
				{
					chosen.push_back( &scene );
				}
			}
		}
	}
	if ( chosen.size() < 2 )
	{
		return {};
	}

	std::vector<Chapter> chapters;
	for ( size_t i = 0; i < chosen.size(); ++i )
	{
		chapters.push_back( {
			chosen[i]->name,
			chosen[i]->chapter,
			i == 0 ? 0 : chosen[i]->start,
			i + 1 < chosen.size() ? chosen[i + 1]->start : total_frames } );
	}
	return chapters;
}

std::string chapter_metadata( const std::vector<Chapter> &chapters, double frame_rate )
{
	const auto milliseconds = [&]( int64_t frame ) { return std::llround( static_cast<double>( frame ) * 1000.0 / frame_rate ); };

	std::ostringstream out;
	out << ";FFMETADATA1\n";
	for ( const Chapter &chapter: chapters )
	{
		out << "[CHAPTER]\nTIMEBASE=1/1000\nSTART=" << milliseconds( chapter.start ) << "\nEND=" << milliseconds( chapter.end ) << "\ntitle=";
		for ( const char c: chapter.title )
		{
			if ( c == '=' || c == ';' || c == '#' || c == '\\' || c == '\n' )
			{
				out << '\\';
			}
			out << c;
		}
		out << '\n';
	}
	return out.str();
}
