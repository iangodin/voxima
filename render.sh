#! /usr/bin/env bash
set -e

mkdir -p render
rm -f render/frame.*  render/output.*

make
build/Release/sonimation --width 1920 --height 1080 \
	--scripts demos/eases --lang fr-FR --theme "Modern Technical" \
	--tts-cache render/tts-cache \
	--output render/output.flac \
	--output render/output.ts \
	--output render/output.mp4

#	--output render/frame.@@@@.svg \
