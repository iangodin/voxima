#! /usr/bin/env bash
# Renders the tutorial's "movie" scene. Extra arguments go to voxima, so a single scene (or several, in order) can be
# rendered instead, and the ones left out of the movie can be reached:
#     ./render.sh --scene what-is-voxima
#     ./render.sh --scene what-is-voxima --scene building-the-project
#     build/Release/voxima --plugin build/Release/tutorial --list-scenes
set -e

mkdir -p render
rm -f render/*.wav render/*.flac render/*.png render/*.svg render/*.mp4 render/*.mkv render/*.webm
rm -f render/frame*.svg render/frame*.png

make
build/Release/voxima --width 1280 --height 720 \
	--plugin build/Release/tutorial \
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
