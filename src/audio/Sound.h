#pragma once

#include "AudioBuffer.h"

#include <chrono>
#include <memory>

// How a sound plays (see DubbingStage::play). Volumes are in dB: 0 is as recorded, -6 about half as loud.
struct SoundOptions
{
	double volume = 0.0;
	bool loop = false;                            // start again at the end, until stopped or faded out
	std::chrono::duration<double> fade_in{ 0.0 }; // rise from silence to the volume over this long
	std::chrono::duration<double> from{ 0.0 };    // start this far into the sound
};

// A sound being played: music, or an effect. It is mixed a block at a time, and ends at its end (unless it
// loops), or when stopped or faded out.
//
// Volume changes are ramps in dB, worked out for every sample, so they do not click or step. A fade from or to
// silence goes from or to SILENT (-60 dB), and the sound is cut off there.
class PlayingSound
{
public:
	static constexpr double SILENT = -60.0;

	// `audio` is stereo, at the rate it will be mixed at
	PlayingSound( std::shared_ptr<const AudioBuffer> audio, const SoundOptions &options );

	// Add the next block of the sound to `block` (stereo, the same rate); nothing once it is done
	void mix_into( AudioBuffer &block );

	bool done() const
	{
		return _done;
	}

	// Go to `db` over `duration` (at once, if zero), from wherever it is
	void set_volume( double db, std::chrono::duration<double> duration = std::chrono::duration<double>{ 0.0 } );

	// Go down to silence over `duration`, then end
	void fade_out( std::chrono::duration<double> duration );

	void stop()
	{
		_done = true;
	}

	// How far into the sound it is, in seconds (for a looping sound, into the current time round)
	double position() const;

	double volume() const
	{
		return _db;
	}

private:
	std::shared_ptr<const AudioBuffer> _audio;
	bool _loop;
	size_t _position = 0; // the next sample frame to play
	bool _done = false;

	double _db;              // the volume now
	double _target_db;       // where a ramp is going
	double _step_db = 0.0;   // how much the volume changes each sample frame while ramping
	size_t _ramp_left = 0;   // sample frames of the ramp to go
	bool _end_at_target = false;

	size_t samples( std::chrono::duration<double> duration ) const;
};

// What a scene holds on to, to change a sound once it has started. Copies share the sound; an empty one (from a
// sound that could not start) does nothing. The sound plays on whether or not anything holds it.
class Sound
{
public:
	Sound() = default;

	explicit Sound( std::shared_ptr<PlayingSound> playing ) :
		_playing( std::move( playing ) )
	{
	}

	void set_volume( double db, std::chrono::duration<double> duration = std::chrono::duration<double>{ 0.0 } )
	{
		if ( _playing )
			_playing->set_volume( db, duration );
	}

	void fade_out( std::chrono::duration<double> duration )
	{
		if ( _playing )
			_playing->fade_out( duration );
	}

	void stop()
	{
		if ( _playing )
			_playing->stop();
	}

	bool done() const
	{
		return !_playing || _playing->done();
	}

	double position() const
	{
		return _playing ? _playing->position() : 0.0;
	}

private:
	std::shared_ptr<PlayingSound> _playing;
};
