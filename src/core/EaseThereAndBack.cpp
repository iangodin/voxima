#include "core/EaseThereAndBack.h"

#include "core/EaseCubic.h"

#include <algorithm>

float ease_there_and_back( float t )
{
	return ease_cubic_in_out( t < 0.5F ? t * 2.F : 2.F - t * 2.F );
}

EaseFunction ease_there_and_back_with_pause( float pause )
{
	const float there = ( 1.F - std::clamp( pause, 0.F, 0.95F ) ) / 2.F;
	return [there]( float t )
	{
		if ( t < there )
		{
			return ease_cubic_in_out( t / there );
		}
		return t > 1.F - there ? ease_cubic_in_out( ( 1.F - t ) / there ) : 1.F;
	};
}
