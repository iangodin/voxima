#include "Microphone.h"
#include "AudioMixer.h"

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
		_mixer.add_audio( buffer );
	}
	else if ( buffer.channels() == 1 )
	{
		AudioBuffer stereo( buffer.frames(), buffer.rate(), 2 );
		const float *src = buffer.data();
		float *dst = stereo.data();
		for ( size_t i = 0; i < buffer.frames(); ++i )
		{
			dst[i * 2] = src[i];
			dst[i * 2 + 1] = src[i];
		}
		_mixer.add_audio( stereo );
	}
}

//====================================//

