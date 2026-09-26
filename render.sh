#! /usr/bin/env bash
# Renders the "movie" scene. Extra arguments go to sonimation, so a single scene (or several, in order) can be
# rendered instead, and the ones left out of the movie can be reached:
#     ./render.sh --scene missing-glyphs
#     ./render.sh --scene morph-shapes --scene karaoke
#     build/Release/sonimation --list-scenes
set -e

mkdir -p render
rm -f render/*.wav render/*.flac render/*.png render/*.svg render/*.mp4 render/*.mkv render/*.webm
rm -f render/frame*.svg render/frame*.png

make
build/Release/sonimation --width 1280 --height 720 \
	--script-dir demos/eases \
	--font-dir assets/fonts \
	--render-dir render \
	--save-svg \
	--theme "New Computer Modern" \
	--container mkv \
	--vcodec h264 \
	--acodec opus \
	--lang en \
	$@
