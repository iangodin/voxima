#! /usr/bin/env bash
set -e

mkdir -p render
rm -f render/*.wav render/*.flac render/*.png render/*.svg render/*.mp4 render/*.mkv render/*.webm
rm -f render/frame*.svg render/frame*.png

make
build/Release/sonimation --width 1280 --height 720 \
	--script-dir demos/eases \
	--font-dir assets/fonts \
	--render-dir render \
	--save-png \
	--theme "Academic Classic" \
	--container mkv \
	--vcodec h264 \
	--acodec opus \
	--lang en \
	--lang fr \
	"$@"
