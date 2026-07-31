#!/usr/bin/env bash

do_render() {
    local output_file="output/$1"
    if [ ! -f "$output_file.png" ]; then
        ./build/wacfrac image -c "$(bash ./scripts/palettes/$2.sh $3)" -f $(cat scripts/poi/$4) -r 3840 2160 -N 250 50 2 -z $5 -o $output_file.ppm
        ffmpeg -i $output_file.ppm $output_file.png
        rm $output_file.ppm
    fi
}

do_render center ultra 24 center 0.4
do_render toughreghnt seafoam 128 big_bang 1e10
do_render ihmolayt fire 128 trees 1e10
do_render wheb snow 64 Sx16 9e10
do_render ghlayshur toyblue 128 infinite_trip 3e2
do_render shanel cottoncandy 128 infinite_trip 1.1e50
do_render luv lipstick 1024 infinite_trip 1e70
do_render blaughsum brain 256 trees 1e20
do_render pghyuke rainbow 1024 big_bang 1e150
