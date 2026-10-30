#include "Retime.h"

#include <algorithm>
#include <cmath>
#include <cstring>

Retimed retime_audio(
	const AudioBuffer &natural,
	const Timeline &timeline,
	const std::vector<double> &starts,
	const std::vector<std::vector<SpeechSpan>> &speech,
	const Stretcher &stretch,
	const CompressOptions &options )
{
	const size_t rate = natural.rate();
	const size_t channels = natural.channels();

	auto to_frames = [&]( double seconds )
	{ return static_cast<size_t>( std::max<long long>( 0, std::llround( seconds * static_cast<double>( rate ) ) ) ); };

	Retimed result;
	result.audio = AudioBuffer( to_frames( starts.back() ), rate, channels );
	result.maps.assign( timeline.segments(), TimeMap{} );
	result.tempos.assign( timeline.segments(), 1.0 );

	const size_t fade = rate * 2 / 1000; // a couple of milliseconds, where silence is added next to speech
	const auto parts = pieces( timeline, starts );
	for ( size_t i = 0; i < parts.size(); ++i )
	{
		const size_t from = std::min( to_frames( parts[i].from ), natural.frames() );
		const size_t to = std::min( to_frames( parts[i].to ), natural.frames() );
		const size_t dest = to_frames( parts[i].dest );
		if ( to <= from || dest >= result.audio.frames() )
		{
			continue;
		}

		// What this segment is given, and what it needs: if that is less, its pauses and then its speech shrink
		const double given = starts[i + 1] - starts[i];
		const double length = parts[i].to - parts[i].from;
		const AudioBuffer *source = nullptr;
		AudioBuffer compressed_audio;
		size_t count = std::min( to - from, result.audio.frames() - dest );
		if ( length > given + 1e-3 )
		{
			AudioBuffer slice( to - from, rate, channels );
			std::memcpy( slice.data(), natural.frame( from ), ( to - from ) * channels * sizeof( float ) );
			const std::vector<SpeechSpan> none;
			Compressed compressed = compress_segment( slice, i < speech.size() ? speech[i] : none, given, stretch, options );
			result.maps[i] = std::move( compressed.map );
			result.tempos[i] = compressed.tempo;
			compressed_audio = std::move( compressed.audio );
			source = &compressed_audio;
			count = std::min( compressed_audio.frames(), result.audio.frames() - dest );
			std::memcpy( result.audio.frame( dest ), source->data(), count * channels * sizeof( float ) );
		}
		else
		{
			std::memcpy( result.audio.frame( dest ), natural.frame( from ), count * channels * sizeof( float ) );
		}

		const bool silence_before = i > 0 && parts[i].dest - ( parts[i - 1].dest + std::min( parts[i - 1].to - parts[i - 1].from, starts[i] - starts[i - 1] ) ) > 1e-9;
		const bool silence_after = source ? false : ( starts[i + 1] - starts[i] ) - length > 1e-9;
		for ( size_t f = 0; f < std::min( fade, count ); ++f )
		{
			const float gain = static_cast<float>( f ) / static_cast<float>( fade );
			for ( size_t c = 0; c < channels; ++c )
			{
				if ( silence_before )
				{
					result.audio.frame( dest + f )[c] *= gain;
				}
				if ( silence_after )
				{
					result.audio.frame( dest + count - 1 - f )[c] *= gain;
				}
			}
		}
	}
	return result;
}

AudioBuffer retime_audio( const AudioBuffer &natural, const Timeline &timeline, const std::vector<double> &starts )
{
	return retime_audio( natural, timeline, starts, {}, Stretcher{} ).audio;
}
