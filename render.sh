#! /usr/bin/env bash
set -e

mkdir -p render
rm -f render/*.svg  render/output.wav render/output.mp4

make
build/Release/sonimation --width 1920 --height 1080 --scripts demos/eases --lang fr-FR --theme "Modern Technical" --tts-cache render/tts-cache --output render/output.wav --output render/output.mp4 --output render/frame_@@@@.svg

#ffmpeg -y -framerate 30 -i render/frame_%04d.png -c:v libx264 -pix_fmt yuv420p -crf 18 render/out.mp4
