#include "Resample.h"
#include <soxr.h>
#include <stdexcept>
#include <string>
#include <cmath>

AudioBuffer resample( const AudioBuffer &input, size_t new_rate )
{
	if ( input.rate() == new_rate )
	{
		return input;
	}

	if ( input.empty() )
	{
		return AudioBuffer( 0, new_rate, input.channels() );
	}

	// Calculate output frames
	double ratio = static_cast<double>( new_rate ) / static_cast<double>( input.rate() );
	size_t out_frames =
		static_cast<size_t>( std::ceil( static_cast<double>( input.frames() ) * ratio ) );

	AudioBuffer output( out_frames, new_rate, input.channels() );

	size_t idone, odone;
	soxr_error_t error = soxr_oneshot(
		static_cast<double>( input.rate() ), // input rate
		static_cast<double>( new_rate ), // output rate
		static_cast<unsigned>( input.channels() ), // channels
		input.data(), // input data
		input.frames(), // input length (frames)
		&idone, // input frames used
		output.data(), // output data
		output.frames(), // output length (frames)
		&odone, // output frames used
		nullptr, // io_spec (default)
		nullptr, // runtime_spec (default)
		nullptr // quality_spec (default)
	);

	if ( error )
	{
		throw std::runtime_error(
			std::string( "Soxr resampling failed: " ) + soxr_strerror( error ) );
	}

	output.resize( odone );

	return output;
}

