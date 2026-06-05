#!/usr/bin/env

ffmpeg -framerate $0 -i $1%05d.ppm -c:v libx264 -pix_fmt yuv420p $1.mp4
