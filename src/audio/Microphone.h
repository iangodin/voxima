
#pragma once

#include "AudioBuffer.h"

#include <utils/ObserverPtr.h>

class AudioMixer;

// Conform speech audio to format expected by mixer
class Microphone
{
public:
	Microphone() = default;

	Microphone( const Microphone &other ) = default;
	Microphone( Microphone &&other ) = default;

	explicit Microphone( AudioMixer &mixer );

	~Microphone() = default;

	Microphone &operator=( const Microphone &other ) = default;
	Microphone &operator=( Microphone &&other ) = default;

	void record( const AudioBuffer &buffer );

	// Delegates to the mixer - see AudioMixer::frames_written().
	size_t frames_written( void ) const;

private:
	ObserverPtr<AudioMixer> _mixer;
	AudioBuffer _buffer;
};
