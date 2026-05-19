#! /usr/bin/env bash
set -e

mkdir -p render
rm -f render/*.wav render/*.flac render/*.png render/*.svg render/*.mp4 render/*.mkv render/*.webm
rm -f render/frame*.svg render/frame*.png

make
build/Release/sonimation --width 1920 --height 1080 \
	--script-dir demos/eases \
	--font-dir assets/fonts \
	--render-dir render \
	--theme "Playful Discovery" \
	--container mkv \
	--vcodec h264 \
	--acodec opus \
	--lang en \
	--lang fr \
#
