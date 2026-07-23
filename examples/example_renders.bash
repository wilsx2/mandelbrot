#!/usr/bin/env

do_render() {
    local output_file="renders/$1"
    if [ ! -f "$output_file.png" ]; then
        ../build/wacfrac -c "$(cat colors/$2)" -f $(cat poi/$3) -r 3840 2160 -N 250 50 2 image -z $4 -o $output_file.ppm
        ffmpeg -i $output_file.ppm $output_file.png
        rm $output_file.ppm
    fi
}

do_render center ultra64 center 0.4
do_render necksus skred128 big_bang 1e10
do_render ihmolayt fire128 trees 1e10
do_render wheb snow64 Sx16 9e10
do_render glayseer toyblue128 infinite_trip 3e2
do_render shanel cottoncandy128 infinite_trip 1.1e50
do_render luv lipstick1024 infinite_trip 1e70
do_render blaughsum brain256 trees 1e20
do_render dandelion throne256 real_line 1e11
do_render rewbee lipstick1024 big_bang 1e150
