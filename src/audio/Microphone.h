
#pragma once

#include "AudioBuffer.h"

class AudioMixer;

class Microphone
{
public:
	Microphone( AudioMixer &mixer );

	void record( const AudioBuffer &buffer );

private:
	AudioMixer &_mixer;
};
