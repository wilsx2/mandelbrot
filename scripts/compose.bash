#!/usr/bin/env

ffmpeg -framerate $1 -i $2_%02d.ppm -c:v libx264 -pix_fmt yuv420p $2.mp4
