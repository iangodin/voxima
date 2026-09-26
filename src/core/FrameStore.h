#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Everything the preview has produced so far, by frame number, held in memory: the SVG of each frame and the
// sound that goes with it. One thread (the producer, which runs the scenes) appends; another (the player)
// reads any frame that exists, in any order, which is what playing backwards, stepping and jumping need. A
// frame is rasterised only when it is shown, so skipping frames costs nothing.
//
// Thread safe: frames are immutable once appended and handed out as shared pointers, so a reader is never
// affected by the store growing.
class FrameStore
{
public:
	struct Frame
	{
		std::string svg;
		std::vector<int16_t> audio; // interleaved stereo, exactly one block
	};

	// block_frames: the stereo sample frames of sound that go with one video frame
	explicit FrameStore( size_t block_frames ) :
		_block_frames( block_frames )
	{
	}

	// samples: interleaved stereo floats, one block. Stored as 16 bit, as the final render is.
	void append( std::string svg, const std::vector<float> &samples )
	{
		auto frame = std::make_shared<Frame>();
		frame->svg = std::move( svg );
		frame->audio.reserve( _block_frames * 2 );
		for ( size_t i = 0; i < _block_frames * 2; ++i )
		{
			const float value = i < samples.size() ? samples[i] : 0.F;
			frame->audio.push_back( static_cast<int16_t>( std::lround( std::clamp( value, -1.F, 1.F ) * 32767.F ) ) );
		}

		std::lock_guard lock( _mutex );
		_frames.push_back( std::move( frame ) );
	}

	// The frames so far
	size_t size() const
	{
		std::lock_guard lock( _mutex );
		return _frames.size();
	}

	// Frame `index`, or null if it has not been made (yet)
	std::shared_ptr<const Frame> frame( int64_t index ) const
	{
		std::lock_guard lock( _mutex );
		if ( index < 0 || static_cast<size_t>( index ) >= _frames.size() )
			return nullptr;
		return _frames[static_cast<size_t>( index )];
	}

	// The last frame there will be has been appended
	bool complete() const
	{
		std::lock_guard lock( _mutex );
		return _complete;
	}

	void finish()
	{
		std::lock_guard lock( _mutex );
		_complete = true;
	}

	// The producer failed: no more frames will come, and the player should report why (see error())
	void fail( std::exception_ptr error )
	{
		std::lock_guard lock( _mutex );
		_error = std::move( error );
		_complete = true;
	}

	std::exception_ptr error() const
	{
		std::lock_guard lock( _mutex );
		return _error;
	}

private:
	size_t _block_frames;
	mutable std::mutex _mutex;
	std::vector<std::shared_ptr<const Frame>> _frames;
	bool _complete = false;
	std::exception_ptr _error;
};
