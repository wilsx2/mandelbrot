#!/usr/bin/env

do_render() {
    local output_file="renders/$1"
    if [ ! -f "$output_file.png" ]; then
        ../build/wacfrac -c "$(cat colors/$2)" -f $(cat poi/$3) -r 3840 2160 image -z $4 -o $output_file.ppm
        ffmpeg -i $output_file.ppm $output_file.png
        rm $output_file.ppm
    fi
}

do_render center ultra64 center 0.4
do_render kaboom fire128 big_bang 1e10
do_render waves lipstick128 trees 1e10
