import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

import wacfracpy as wf

def main():
    res = wf.video_resolution.SD360p
    base_view = wf.viewport(0.0 + 1.0j, 2.5 + 2.5j)
    plots = []
    for i in range(0, 100):
        zoom = 10**(i/2)
        base_view.precision(int(10 + i))
        view = base_view.zoomed(zoom)
        plots.append(wf.plot(res, view, 256))
    wf.save_to_ppm("strand", plots)

if __name__ == "__main__":
    main()
