#pragma once

#include <cstdint>
#include <optional>
#include <string>

// A place in the movie, for --start-at and --stop-at: a frame number if it is all digits, or else the name of a scene
struct PlayPoint
{
	std::optional<int64_t> frame;
	std::string scene;

	static PlayPoint parse( const std::string &text );

	[[nodiscard]] std::string describe() const;
};

// Which part of the movie is wanted, followed as it plays. The scenes always run from the start, so what comes before the part is
// there, as it would be; but nothing of it is drawn or written. A start is where a scene starts, or that frame. A stop is included:
// the end of that scene, or that frame. A scene that is played more than once counts the first time (for a stop, the first
// time after the start).
class PlayRange
{
public:
	PlayRange() = default;
	PlayRange( std::optional<PlayPoint> start, std::optional<PlayPoint> stop );

	// Told by Stage::scene_event, which happens during a tick
	void scene_event( const std::string &name, bool started );

	// Told after each tick, with the number of the frame it made. The frame is wanted unless skipping() or stopping().
	void frame_made( int64_t frame );

	// Not yet at the start
	[[nodiscard]] bool skipping() const
	{
		return !_started;
	}

	// Past the stop
	[[nodiscard]] bool stopping() const
	{
		return _stopping;
	}

	[[nodiscard]] const std::optional<PlayPoint> &start() const
	{
		return _start;
	}

	[[nodiscard]] const std::optional<PlayPoint> &stop() const
	{
		return _stop;
	}

private:
	std::optional<PlayPoint> _start;
	std::optional<PlayPoint> _stop;
	bool _started = true;
	bool _stopping = false;
};
