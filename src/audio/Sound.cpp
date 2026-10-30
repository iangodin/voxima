#include "Sound.h"

#include "Volume.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

PlayingSound::PlayingSound( std::shared_ptr<const AudioBuffer> audio, const SoundOptions &options ) :
	_audio( std::move( audio ) ),
	_loop( options.loop ),
	_db( options.volume ),
	_target_db( options.volume )
{
	if ( !_audio || _audio->channels() != 2 )
	{
		throw std::invalid_argument( "a sound to play must be stereo" );
	}

	_position = std::min( samples( options.from ), _audio->frames() );
	if ( _audio->frames() == 0 )
	{
		_done = true;
	}

	if ( options.fade_in.count() > 0.0 )
	{
		_db = SILENT;
		set_volume( options.volume, options.fade_in );
	}
}

size_t PlayingSound::samples( std::chrono::duration<double> duration ) const
{
	return static_cast<size_t>( std::llround( std::max( 0.0, duration.count() ) * static_cast<double>( _audio->rate() ) ) );
}

void PlayingSound::set_volume( double db, std::chrono::duration<double> duration )
{
	_end_at_target = false;
	_target_db = db;
	_ramp_left = samples( duration );
	if ( _ramp_left == 0 )
	{
		_db = db;
		_step_db = 0.0;
		return;
	}
	_step_db = ( db - _db ) / static_cast<double>( _ramp_left );
}

void PlayingSound::fade_out( std::chrono::duration<double> duration )
{
	set_volume( std::min( SILENT, _db ), duration );
	_end_at_target = true;
	if ( _ramp_left == 0 )
	{
		_done = true;
	}
}

double PlayingSound::position() const
{
	return static_cast<double>( _position ) / static_cast<double>( _audio->rate() );
}

void PlayingSound::mix_into( AudioBuffer &block )
{
	if ( block.channels() != 2 || block.rate() != _audio->rate() )
	{
		throw std::invalid_argument( "a sound is mixed into a stereo block at its own rate" );
	}

	for ( size_t f = 0; f < block.frames() && !_done; ++f )
	{
		if ( _position >= _audio->frames() )
		{
			if ( !_loop )
			{
				_done = true;
				break;
			}
			_position = 0;
		}

		if ( _ramp_left > 0 )
		{
			_db += _step_db;
			if ( --_ramp_left == 0 )
			{
				_db = _target_db;
				if ( _end_at_target )
				{
					_done = true;
					break;
				}
			}
		}

		const float gain = _db <= SILENT ? 0.F : db_to_gain( static_cast<float>( _db ) );
		const float *in = _audio->frame( _position );
		float *out = block.frame( f );
		out[0] += in[0] * gain;
		out[1] += in[1] * gain;
		++_position;
	}
}
