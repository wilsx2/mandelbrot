#!/usr/bin/env

cmd=""
if [[ $(gum choose --header="Render Type: " image video) == "image" ]]; then
    poi=$(gum file --padding="1 0 0 0" --header="Focus Point:" ./poi)
    zoom=$(gum input --header="Zoom Factor: (Scientific notation accepted. Larger zooms take longer to render)" --placeholder="0.4")
    palette=$(gum file --padding="1 0 0 0" --header="Color Palette:" ./colors)
    resolution=$(gum choose --header="Resolution:" "100 100" "500 500" "720 1280" "1280 720" "1920 1080" "3840 2160")
    output=$(gum input --header="Output File:" --placeholder="mandelbrot.ppm")
    cmd="../build/wacfrac image -f $(cat $poi) -z $zoom -c \"$(cat $palette)\" -r $resolution -o $output"
else
    poi=$(gum file --padding="1 0 0 0" --header="Focus Point:" ./poi)
    initial=$(gum input --header="Initial Zoom: (Scientific notation accepted. Larger zooms take longer to render)" --placeholder="0.4")
    final=$(gum input --header="Final Zoom:" --placeholder="1e10")
    zps=$(gum input --header="Zoom Per Second:" --placeholder="2")
    palette=$(gum file --padding="1 0 0 0" --header="Color Palette:" ./colors)
    fps=$(gum input --header="Frames Per Second:" --placeholder="24")
    resolution=$(gum choose --header="Resolution:" "100 100" "500 500" "720 1280" "1280 720" "1920 1080" "3840 2160")
    output=$(gum input --header="Project Name:" --placeholder="mandelbrot")
    cmd="../build/wacfrac video -f $(cat $poi) -a $initial -b $final -z $zps --fps $fps -c \"$(cat $palette)\" -r $resolution -o $output"
fi

echo $cmd
$cmd
