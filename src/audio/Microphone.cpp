#include "Microphone.h"
#include "AudioMixer.h"

#include <stdexcept>

//====================================//

Microphone::Microphone( AudioMixer &mixer ) :
	_mixer( mixer )
{
}

//====================================//

void Microphone::record( const AudioBuffer &buffer )
{
	if ( buffer.channels() == 2 )
	{
		_mixer->add_audio( buffer );
	}
	else if ( buffer.channels() == 1 )
	{
		if ( _buffer.empty() )
		{
			_buffer = AudioBuffer( buffer.frames(), buffer.rate(), 2 );
		}
		if ( buffer.frames() != _buffer.frames() )
		{
			throw std::runtime_error( "speaker changed audio block size" );
		}
		const float *src = buffer.data();
		float *dst = _buffer.data();
		for ( size_t i = 0; i < buffer.frames(); ++i )
		{
			dst[i * 2] = src[i];
			dst[i * 2 + 1] = src[i];
		}
		_mixer->add_audio( _buffer );
	}
}

//====================================//

