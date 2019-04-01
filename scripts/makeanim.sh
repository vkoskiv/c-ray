#!/bin/bash

# Create animation from timelapse frames
# vkoskiv, 30.3.2019
# -pix_fmt yuv420p is required for it to play on quicktime

DATE=$(date "+%Y-%m-%d-%H%M")

#ffmpeg -f concat -i list.txt -r 30 -vcodec libx264 -pix_fmt yuv420p final-$DATE.mp4
ffmpeg -r 25 -i ../output/animation/rendered_%04d.png -vcodec libx264 -crf 25 -pix_fmt yuv420p animation-$DATE.mp4
