
#pragma once

#include <cstddef>
#include <vector>

class AudioBuffer
{
public:
	AudioBuffer() = default;

	/// @brief Constructor
	///
	/// One frame is a pair of left/right float values.
	///
	/// @param frames Number of frames in buffer
	AudioBuffer( size_t frames, size_t rate, size_t channels )
		: _data( frames * channels ), _frames( frames ), _channels( channels ), _rate( rate )
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

	bool same( const AudioBuffer &other ) const
	{
		return _frames == other._frames && _channels == other._channels && _rate == other._rate;
	}

	size_t channels( void ) const
	{
		return _channels;
	}

	const float *frame( size_t index ) const
	{
		return &_data.at( index * _channels );
	}

	float *frame( size_t index )
	{
		return &_data.at( index * _channels );
	}

	const float *begin( void ) const
	{
		return _data.data();
	}

	float *begin( void )
	{
		return _data.data();
	}

	const float *end( void ) const
	{
		return _data.data() + _data.size();
	}

	float *end( void )
	{
		return _data.data() + _data.size();
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

	double duration( void ) const
	{
		return static_cast<double>( _frames ) / static_cast<double>( _rate );
	}

private:
	std::vector<float> _data;
	size_t _frames = 0;
	size_t _channels = 0;
	size_t _rate = 0;
};
