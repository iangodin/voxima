
#pragma once

#include <vector>
#include <cstddef>

class AudioBuffer
{
public:
	/// @brief Constructor
	///
	/// One frame is a pair of left/right float values.
	///
	/// @param frames Number of frames in buffer
	AudioBuffer( size_t frames, size_t rate = 48000, size_t channels = 2 ) :
		_data( frames * channels ), _frames( frames ), _channels( channels ), _rate( rate )
	{
	}

	bool empty( void ) const
	{
		return _data.empty();
	}

	size_t rate( void ) const
	{
		return _rate;
	}

	size_t frames( void ) const
	{
		return _frames;
	}

	size_t channels( void ) const
	{
		return _channels;
	}

	const float *frame( size_t index ) const
	{
		return &_data.at( index * _channels );
	}

	size_t samples( void ) const
	{
		return _data.size();
	}

	const float *data( void ) const
	{
		return _data.data();
	}

	float *data( void )
	{
		return _data.data();
	}

	void resize( size_t frames )
	{
		_data.resize( frames * _channels );
		_frames = frames;
	}

private:
	std::vector<float> _data;
	size_t _frames = 0;
	size_t _channels = 0;
	size_t _rate = 48000;
};

