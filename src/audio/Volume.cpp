#include "Volume.h"

#include <algorithm>
#include <cmath>

void apply_volume( AudioBuffer &buffer, float gain )
{
	std::transform( buffer.begin(), buffer.end(), buffer.begin(), [gain]( float sample )
	{
		return sample * gain;
	} );
}

float db_to_gain( float db )
{
	return std::pow( 10.F, db / 20.F );
}
