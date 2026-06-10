import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

import wacfracpy as wf

def main():
    res = wf.video_resolution.SD360p
    base_view = wf.viewport(-2.0 - 1.5j, 1.0 + 1.5j)
    focus = wf.poi.BIG_BANG
    plots = []
    for i in range(0, 50):
        zoom = 10**i
        base_view.precision(int(10 + i/2.5))
        view = base_view.at_zoom(focus, zoom)
        plots.append(wf.plot(res, view, 256 * i))

    wf.save_to_ppm("bang", plots)

if __name__ == "__main__":
    main()
