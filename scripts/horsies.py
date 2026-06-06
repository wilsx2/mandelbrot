import mplotpy as mp

def main():
    width = 1920
    height = 1080
    plots = [];
    base_limit = mp.axis_limits(-2.0 - 1.5j, 1.0 + 1.5j);

    for i in range(0, 16):
        plots.append(mp.plot( width, height, base_limit.at_zoom(-2.0 + 0.0j, 2**i) ));
    mp.save_plots("horsies", plots);

if __name__ == "__main__":
    main()
