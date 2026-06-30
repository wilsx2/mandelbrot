#!/usr/bin/env

if [[ $(gum choose --header="Choose render mode: " image video) == "image" ]]; then
    # Image, later change "perturbed" to auto
    ../build/wacfrac perturbed -f $(cat $(gum file --padding="1 0 0 0" --header="Focus Point:" ./poi)) -z $(gum input --header="Zoom Factor: (Scientific notation accepted)" --placeholder="0.4") -c $(cat $(gum file --padding="1 0 0 0" --header="Color Palette:" ./colors)) -o $(gum input --header="Output File:" --placeholder="mandelbrot.ppm") -s -e 4 -n 256
#else
    # Video
fi
