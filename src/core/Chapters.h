#pragma once

#include <cstdint>
#include <string>
#include <vector>

// The chapters of a video, as players and YouTube show them: which of the scenes played are chapters, and how they are written down
// for ffmpeg.

// A scene that was played, in the frames of the output
struct PlayedScene
{
	std::string name;
	std::string chapter; // the name it was marked as a chapter with (PlayOptions::chapter), if it was
	int depth = 0;       // 0 for the outermost; a scene played from inside another is one deeper
	int64_t start = 0;
	int64_t end = -1;    // -1 while it is still running
};

struct Chapter
{
	std::string name; // the scene's
	std::string title;
	int64_t start = 0; // in frames
	int64_t end = 0;
};

// Which of the scenes are chapters. If any was marked as one, only those are, with the titles they were marked with. Otherwise it
// is the top-level scenes of the movie: the outermost ones that are more than one, so a movie that plays parts that
// play sections has its parts as chapters (and `title` is left for the caller). Scenes of no length (those a --start-at went past) are
// left out. Each lasts until the next starts, the first from the very start and the last until `total_frames`. Fewer than two
// is no chapters at all.
std::vector<Chapter> choose_chapters( const std::vector<PlayedScene> &scenes, int64_t total_frames );

// The chapters in ffmpeg's metadata format (-i file -map_chapters), at this many frames a second
std::string chapter_metadata( const std::vector<Chapter> &chapters, double frame_rate );
