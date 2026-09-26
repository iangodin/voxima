#include "Typing.h"

#include <core/Tween.h>
#include <visuals/Glyph.h>

#include <algorithm>

namespace
{

// A small random number generator of our own (splitmix64): std's distributions give different numbers with different
// standard libraries, and a render must come out the same everywhere
class Random
{
public:
	explicit Random( uint64_t seed ) :
		_state( seed )
	{
	}

	// From 0 up to (not including) 1
	double next()
	{
		uint64_t z = ( _state += 0x9E3779B97F4A7C15ULL );
		z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
		z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBULL;
		z ^= z >> 31;
		return static_cast<double>( z >> 11 ) * 0x1.0p-53;
	}

private:
	uint64_t _state;
};

// Letters, digits and the underscore, and anything beyond ASCII: what a word or a name is made of
bool is_word( char c )
{
	const auto u = static_cast<unsigned char>( c );
	return u >= 0x80 || ( u >= '0' && u <= '9' ) || ( u >= 'a' && u <= 'z' ) || ( u >= 'A' && u <= 'Z' ) || c == '_';
}

// How much longer than usual the gap before typing `c` is, after `previous`
double pause( char previous, char c, Random &random )
{
	if ( previous == '\n' )
	{
		if ( c == '\n' )
		{
			return 1.5; // a blank line
		}
		// The end of a line: a clear pause, and now and then a longer one, thinking what comes next
		return random.next() < 0.15 ? 8.0 : 4.0;
	}
	switch ( previous )
	{
		case ';':
		case ',':
		case ')':
		case '}':
		case '{': return 2.0;
		case ' ': return 1.5;
		default: break;
	}
	return is_word( previous ) && is_word( c ) ? 0.8 : 1.0;
}

}

std::vector<double> typing_schedule( std::string_view text, const TypingRhythm &rhythm )
{
	std::vector<double> times( text.size(), 0.0 );
	Random random( rhythm.seed );

	double now = 0.0;
	size_t keys = 0;
	char previous = 0;
	bool indenting = true; // at the start of a line, before anything but spaces
	for ( size_t i = 0; i < text.size(); ++i )
	{
		const char c = text[i];
		const bool continuation = ( static_cast<unsigned char>( c ) & 0xC0 ) == 0x80;
		if ( continuation )
		{
			times[i] = now; // part of the character before
			continue;
		}

		if ( indenting && c == ' ' )
		{
			times[i] = now; // an editor indents: it comes with the line
			continue;
		}
		indenting = c == '\n';

		if ( keys > 0 )
		{
			double gap = 1.0;
			if ( rhythm.human )
			{
				gap = pause( previous, c, random ) * ( 0.7 + 0.7 * random.next() );
			}
			now += gap;
		}
		times[i] = now;
		previous = c;
		++keys;
	}

	// So many characters take so long, on average, whatever the pauses
	if ( keys > 1 && now > 0.0 )
	{
		const double scale = ( static_cast<double>( keys - 1 ) / rhythm.chars_per_second ) / now;
		for ( double &t: times )
		{
			t *= scale;
		}
	}
	return times;
}

Typewriter::Typewriter( CodeListing &listing, const TypingRhythm &rhythm ) :
	_fade( std::max( 0.0, rhythm.fade_seconds ) )
{
	const std::vector<double> times = typing_schedule( listing.text, rhythm );
	double last = 0.0;
	for ( const CodeGlyph &g: listing.glyphs )
	{
		const double time = g.offset < times.size() ? times[g.offset] : 0.0;
		_keys.push_back( { g.glyph, time, g.glyph->color.a } );
		last = std::max( last, time );
	}
	_duration = _keys.empty() ? 0.0 : last + _fade;
	set_time( 0.0 );
}

void Typewriter::set_time( double seconds )
{
	for ( Key &key: _keys )
	{
		double shown = 0.0;
		if ( _fade > 0.0 )
		{
			shown = std::clamp( ( seconds - key.time ) / _fade, 0.0, 1.0 );
		}
		else
		{
			shown = seconds >= key.time ? 1.0 : 0.0;
		}
		key.glyph->color.a = key.alpha * static_cast<float>( shown );
	}
}

Scene Typewriter::type( const Stage &stage )
{
	return type( stage, std::chrono::duration<double>( _duration ) );
}

Scene Typewriter::type( const Stage &stage, std::chrono::duration<double> duration )
{
	co_await tween( stage, duration, []( float t ) { return t; }, [this]( float t ) { set( t ); } );
}
