#! /usr/bin/env bash
# Renders the "movie" scene. Extra arguments go to voxima, so a single scene (or several, in order) can be
# rendered instead, and the ones left out of the movie can be reached:
#     ./render.sh --scene missing-glyphs
#     ./render.sh --scene morph-shapes --scene karaoke
#     build/Release/voxima --plugin build/Release/eases --list-scenes
set -e

mkdir -p render
rm -f render/*.wav render/*.flac render/*.png render/*.svg render/*.mp4 render/*.mkv render/*.webm
rm -f render/frame*.svg render/frame*.png

make
build/Release/tutorial_demo --width 1280 --height 720 \
	--script-dir demos/tutorial \
	--font-dir assets/fonts \
	--theme-dir themes \
	--render-dir render \
	--save-svg \
	--container mkv \
	--vcodec h264 \
	--acodec opus \
	--lang en \
	--theme modern-technical  \
	"$@"
