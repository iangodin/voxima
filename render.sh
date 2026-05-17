#! /usr/bin/env bash
set -e

mkdir -p render
rm -f render/frame.*  render/output.*

make
build/Release/sonimation --width 1920 --height 1080 \
	--scripts demos/eases --lang en --theme "Modern Technical" \
	--render-dir render \
	--vcodec h264 \
	--output render/output.mkv \
	--output render/output.mp4

#	--output render/frame.@@@@.svg \
