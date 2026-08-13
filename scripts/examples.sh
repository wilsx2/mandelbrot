#!/usr/bin/env bash

do_render() {
    mkdir output
    local output_file="output/$1"
    if [ ! -f "$output_file.png" ]; then
        wacfrac image -c "$(bash palettes/$2.sh $3)" -f $(cat poi/$4) -r 3840 2160 -N 250 50 2 -z $5 -o $output_file.ppm
        ffmpeg -i $output_file.ppm $output_file.png
        rm $output_file.ppm
    fi
}

do_render center ultra 24 center 0.4
do_render torrent seafoam 128 big_bang 1e10
do_render immolate fire 128 trees 1e10
do_render web snow 64 Sx16 9e10
do_render glacier toyblue 128 infinite_trip 3e2
do_render channel cottoncandy 128 infinite_trip 1.1e50
do_render love lipstick 1024 infinite_trip 1e70
do_render blossom brain 256 trees 1e20
do_render nexus rainbow 1024 big_bang 1e150
