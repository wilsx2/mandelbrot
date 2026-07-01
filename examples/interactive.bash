#!/usr/bin/env

cmd=""
if [[ $(gum choose --header="Render Type: " image video) == "image" ]]; then
    poi=$(cat $(gum file --padding="1 0 0 0" --header="Focus Point:" ./poi))
    zoom=$(gum input --header="Zoom Factor: (Scientific notation accepted. Larger zooms take longer to render)" --placeholder="0.4")
    palette=$(cat $(gum file --padding="1 0 0 0" --header="Color Palette:" ./colors))
    resolution=$(gum choose --header="Resolution:" "100 100" "500 500" "720 1280" "1280 720" "1920 1080" "3840 2160")
    output=$(gum input --header="Output File:" --placeholder="mandelbrot.ppm")
    cmd="../build/wacfrac perturbed -f $poi -z $zoom -c $palette -r $resolution -o $output -s -e 4 && feh $output -F -Z"
else
    poi=$(cat $(gum file --padding="1 0 0 0" --header="Focus Point:" ./poi))
    initial=$(gum input --header="Initial Zoom: (Scientific notation accepted. Larger zooms take longer to render)" --placeholder="0.4")
    final=$(gum input --header="Final Zoom:" --placeholder="1e10")
    zps=$(gum input --header="Zoom Per Second:" --placeholder="2")
    palette=$(cat $(gum file --padding="1 0 0 0" --header="Color Palette:" ./colors))
    fps=$(gum input --header="Frames Per Second:" --placeholder="24")
    resolution=$(gum choose --header="Resolution:" "100 100" "500 500" "720 1280" "1280 720" "1920 1080" "3840 2160")
    output=$(gum input --header="Project Name:" --placeholder="mandelbrot")
    cmd="../build/wacfrac video -f $poi -a $initial -b $final -z $zps -c $palette --fps $fps  -r $resolution -o $output -s -e 4"
fi

echo $cmd
$cmd
