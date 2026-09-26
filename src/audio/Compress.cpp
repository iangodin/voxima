#include "Compress.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace
{

struct Region
{
	size_t from;       // frames in the original segment
	size_t to;
	bool speech;
	size_t kept = 0;   // frames left of it
};

// Fade the first `count` frames in, or the last `count` frames out, of `count` frames at `at`
void fade( AudioBuffer &audio, size_t at, size_t count, bool in )
{
	for ( size_t f = 0; f < count; ++f )
	{
		const float gain = static_cast<float>( in ? f : count - 1 - f ) / static_cast<float>( std::max<size_t>( count, 1 ) );
		for ( size_t c = 0; c < audio.channels(); ++c )
		{
			audio.frame( at + f )[c] *= gain;
		}
	}
}

}

Compressed compress_segment(
	const AudioBuffer &segment,
	const std::vector<SpeechSpan> &speech,
	double target_seconds,
	const Stretcher &stretch,
	const CompressOptions &options )
{
	const size_t rate = segment.rate();
	const size_t channels = segment.channels();
	auto to_frames = [&]( double seconds )
	{ return static_cast<size_t>( std::max<long long>( 0, std::llround( seconds * static_cast<double>( rate ) ) ) ); };
	auto to_seconds = [&]( size_t frames ) { return static_cast<double>( frames ) / static_cast<double>( rate ); };

	const size_t target = to_frames( target_seconds );
	Compressed result;
	result.audio = AudioBuffer( target, rate, channels ); // silence
	if ( segment.frames() == 0 || target == 0 )
	{
		return result;
	}

	// Already short enough: as it is, and silence after it
	if ( segment.frames() <= target )
	{
		std::memcpy( result.audio.frame( 0 ), segment.data(), segment.frames() * channels * sizeof( float ) );
		return result;
	}

	// The segment as speech and pauses, in order
	std::vector<SpeechSpan> spans;
	for ( auto span: speech )
	{
		span.start = std::clamp( span.start, 0.0, to_seconds( segment.frames() ) );
		span.end = std::clamp( span.end, span.start, to_seconds( segment.frames() ) );
		if ( span.end > span.start )
		{
			spans.push_back( span );
		}
	}
	std::sort( spans.begin(), spans.end(), []( const SpeechSpan &a, const SpeechSpan &b ) { return a.start < b.start; } );

	std::vector<Region> regions;
	size_t cursor = 0;
	for ( const auto &span: spans )
	{
		const size_t from = std::max( cursor, to_frames( span.start ) );
		const size_t to = std::min( segment.frames(), std::max( from, to_frames( span.end ) ) );
		if ( to <= from )
		{
			continue; // inside the one before
		}
		if ( from > cursor )
		{
			regions.push_back( { cursor, from, false } );
		}
		regions.push_back( { from, to, true } );
		cursor = to;
	}
	if ( cursor < segment.frames() )
	{
		regions.push_back( { cursor, segment.frames(), false } );
	}

	// How much has to go, and how much of that the pauses can give without going below the floor
	const size_t floor_frames = to_frames( options.pause_floor );
	const size_t need = segment.frames() - target;
	size_t available = 0;
	for ( const auto &region: regions )
	{
		if ( !region.speech && region.to - region.from > floor_frames )
		{
			available += region.to - region.from - floor_frames;
		}
	}
	const size_t shrink = std::min( need, available );

	// Each pause gives up its share of that
	size_t shrunk = 0;
	for ( auto &region: regions )
	{
		const size_t length = region.to - region.from;
		region.kept = length;
		if ( !region.speech && length > floor_frames && available > 0 )
		{
			const size_t give = static_cast<size_t>( std::llround( static_cast<double>( shrink ) * static_cast<double>( length - floor_frames ) / static_cast<double>( available ) ) );
			region.kept = length - std::min( give, length - floor_frames );
		}
		shrunk += length - region.kept;
	}

	// The shortened audio: a pause loses its middle, so what is left of it is the two ends
	const size_t short_frames = segment.frames() - shrunk;
	AudioBuffer shortened( short_frames, rate, channels );
	std::vector<std::pair<double, double>> knots{ { 0.0, 0.0 } };
	size_t out = 0;
	const size_t join = rate * 2 / 1000; // a couple of milliseconds, where the middle of a pause is taken out
	for ( const auto &region: regions )
	{
		const size_t length = region.to - region.from;
		if ( region.kept == length )
		{
			std::memcpy( shortened.frame( out ), segment.frame( region.from ), length * channels * sizeof( float ) );
		}
		else if ( region.kept > 0 )
		{
			const size_t front = region.kept / 2;
			const size_t back = region.kept - front;
			if ( front > 0 )
			{
				std::memcpy( shortened.frame( out ), segment.frame( region.from ), front * channels * sizeof( float ) );
				fade( shortened, out, std::min( join, front ), false );
			}
			std::memcpy( shortened.frame( out + front ), segment.frame( region.to - back ), back * channels * sizeof( float ) );
			fade( shortened, out + front, std::min( join, back ), true );
		}
		out += region.kept;
		knots.emplace_back( to_seconds( region.to ), to_seconds( out ) );
	}

	// What is still too long is the speech itself, speeded up by that little
	AudioBuffer done = std::move( shortened );
	double tempo = 1.0;
	if ( done.frames() > target + rate / 1000 )
	{
		tempo = static_cast<double>( done.frames() ) / static_cast<double>( target );
		if ( !stretch )
		{
			throw std::runtime_error( "compress_segment: the speech has to be speeded up and there is no stretcher" );
		}
		done = stretch( done, tempo );
		for ( auto &knot: knots )
		{
			knot.second /= tempo;
		}
	}

	std::memcpy( result.audio.frame( 0 ), done.data(), std::min( done.frames(), target ) * channels * sizeof( float ) );
	result.map = TimeMap( std::move( knots ) );
	result.tempo = tempo;
	return result;
}
