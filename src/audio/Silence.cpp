#include "Silence.h"

#include <cmath>

AudioBuffer make_silence( size_t frames, size_t rate, size_t channels )
{
	return AudioBuffer( frames, rate, channels );
}

AudioBuffer make_silence_for_duration( double duration, size_t rate, size_t channels )
{
	size_t frames = static_cast<size_t>( std::llround( duration * static_cast<double>( rate ) ) );
	return make_silence( frames, rate, channels );
}
