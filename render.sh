#! /usr/bin/env bash

mkdir -p frames
i=0
for f in frames/frame_*.svg; do
  printf -v idx "%04d" $i
  rsvg-convert "$f" -o frames/frame_$idx.png
  i=$((i+1))
done

ffmpeg -y -framerate 30 -i frames/frame_%04d.png -c:v libx264 -pix_fmt yuv420p -crf 18 frames/out.mp4
