#include "Retime.h"

#include <algorithm>
#include <cmath>
#include <cstring>

AudioBuffer retime_audio( const AudioBuffer &natural, const Timeline &timeline, const std::vector<double> &starts )
{
	const size_t rate = natural.rate();
	const size_t channels = natural.channels();

	auto to_frames = [&]( double seconds )
	{ return static_cast<size_t>( std::max<long long>( 0, std::llround( seconds * static_cast<double>( rate ) ) ) ); };

	AudioBuffer out( to_frames( starts.back() ), rate, channels );
	const size_t fade = rate * 2 / 1000; // a couple of milliseconds, where silence is added next to speech
	const auto parts = pieces( timeline, starts );
	for ( size_t i = 0; i < parts.size(); ++i )
	{
		const size_t from = std::min( to_frames( parts[i].from ), natural.frames() );
		const size_t to = std::min( to_frames( parts[i].to ), natural.frames() );
		const size_t dest = to_frames( parts[i].dest );
		if ( to <= from || dest >= out.frames() )
		{
			continue;
		}
		const size_t count = std::min( to - from, out.frames() - dest );
		std::memcpy( out.frame( dest ), natural.frame( from ), count * channels * sizeof( float ) );

		const bool silence_before = i > 0 && parts[i].dest - ( parts[i - 1].dest + ( parts[i - 1].to - parts[i - 1].from ) ) > 1e-9;
		const bool silence_after = ( starts[i + 1] - starts[i] ) - ( parts[i].to - parts[i].from ) > 1e-9;
		for ( size_t f = 0; f < std::min( fade, count ); ++f )
		{
			const float gain = static_cast<float>( f ) / static_cast<float>( fade );
			for ( size_t c = 0; c < channels; ++c )
			{
				if ( silence_before )
				{
					out.frame( dest + f )[c] *= gain;
				}
				if ( silence_after )
				{
					out.frame( dest + count - 1 - f )[c] *= gain;
				}
			}
		}
	}
	return out;
}
