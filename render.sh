#! /usr/bin/env bash
set -e

mkdir -p frames
rm -f frames/*

make
build/Release/math_test

ffmpeg -y -framerate 30 -i frames/frame_%04d.png -c:v libx264 -pix_fmt yuv420p -crf 18 frames/out.mp4
