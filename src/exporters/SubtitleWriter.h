#pragma once

#include <audio/SubtitleCue.h>

#include <string>
#include <vector>

// Writes subtitle cues as an SRT file (HH:MM:SS,mmm timestamps).
void write_srt( const std::string &path, const std::vector<SubtitleCue> &cues );

// Writes subtitle cues as a WebVTT file (HH:MM:SS.mmm timestamps, WEBVTT header).
void write_vtt( const std::string &path, const std::vector<SubtitleCue> &cues );
