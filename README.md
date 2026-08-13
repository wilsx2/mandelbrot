# wacfrac

## Overview

wacfrac is a Mandelbrot set renderer which leverages various numerical
techniques massively in parallel for high speed renders with extreme precision.

## Examples

![Screenshot](./images/center.png)
![Screenshot](./images/glacier.png)
![Screenshot](./images/torrent.png)
![Screenshot](./images/immolate.png)
![Screenshot](./images/web.png)
![Screenshot](./images/blossom.png)
![Screenshot](./images/channel.png)
![Screenshot](./images/love.png)
![Screenshot](./images/nexus.png)

*From top to bottom:*

| Render | Palette | Location | Zoom |
| --- | --- | --- | --- |
| center | ultra | center of the set | 0.4 |
| glacier | toyblue | infinite_trip | 3e2 |
| torrent | seafoam | big_bang | 1e10 |
| immolate | fire | trees | 1e10 |
| web | snow | Sx16 | 9e10 |
| blossom | brain | trees | 1e20 |
| channel | cottoncandy | infinite_trip | 1.1e50 |
| love | lipstick | infinite_trip | 1e70 |
| nexus | rainbow | big_bang | 1e150 |

## Features

- **GPU-accelerated rendering** built on oneAPI SYCL, with an optional CUDA backend.
  Parallelism not just for rendering, but for approximation construction
  and component-wise arbitrary precision calculations.
- **Extreme deep-zoom support** via arbitrary-precision arithmetic (GMP/MPFR/MPC)
  plus a custom floating-exponent type (FloatExp) that preserves a full
  double-precision mantissa across hundreds of orders of magnitude of zoom.
- **Two rendering strategies**
  - *Pertubation Theory* rendering which computes pixel orbits relatively to a
    reference orbit. Reduces required precision for per-pixel computations,
    enabling fast fixed precision arithmetic where arbitrary precision would
    otherwise be necessary.
  - *Bilinear approximation (BLA)* which batches many thousands of iterations into
    a single compution, retaining visual accuracy and greatly accelerating renders.
- **Docker images** available on Docker Hub for reproducable distribution.
Read about the implementation details and reasoning behind these features [here](docs/document.pdf).

## Benchmarks

Benchmarks were taken focused on the big bang POI with otherwise default parameters.
The italic column indicates the fastest render mode.
The bold column indicates the mode automatically selected when none is specified.
Showing wall time as reported by the 'time' utility. 'user' time would bias benchmarks
heavily towards GPUs.

| Zoom | Processor | Direct Mode | Perturbed Mode | Approximate Mode |
| --- | --- | --- | --- | --- |
| 1e10 |  Intel i7, 8th Gen | *0.357s* | **0.601s** | 1.304s |
| 1e250 | Intel i7, 8th Gen | N/A | *45.612s* | **1m 48.047** |
| 1e500 | Intel i7, 8th Gen | N/A | 16m 16.793s | ***6m 22.969s*** |
| 1e10 | NVIDIA GeForce RTX 3060 | 0.334s | ***0.259s*** | 0.321s |
| 1e250 | NVIDIA GeForce RTX 3060 | N/A | 3.2480 | ***2.313s*** |
| 1e500 | NVIDIA GeForce RTX 3060 | N/A | 29.793s | ***23.302s*** |

## Deployment

wacfrac can be found built with all its dependencies on [Docker Hub](https://hub.docker.com/r/wilsx2/wacfrac).

```bash
docker pull wilsx2/wacfrac:latest
```

If run without any flags Docker will not be able to access your GPU.
The flags required depend on your OS and GPU.

```bash
# NVIDIA, any OS (requires NVIDIA Container Toolkit)
docker run --gpus all -it wilsx2/wacfrac:latest
# Intel, Linux
docker run --devices=/dev/dri -it wilsx2/wacfrac:latest
# Intel, Windows
docker run --devices=/dev/dxg -it wilsx2/wacfrac:latest
```

Note: wacfrac has been tested on Intel iGPUs, AMD iGPUs, and a NVIDIA RTX 3060.
Discrete Intel GPUs are supported but untested, while AMD hardware is unsupported.

It is recommended you create a link between your container and the host so you
can easily access your renders.

```bash
docker run [GPU flags] --mount type=bind,src=./output,dst=~/output -it wilsx2/wacfrac:latest
```

## Usage

wacfrac is a CLI tool. It can be run in image mode or video mode. Currently
video mode is experimental.

### Basics, Image Mode

With no additional flags wacfrac will emit a 500x500 render of the complete
Mandelbrot set to an image called "mandelbrot.ppm".

```bash
wacfrac image
```

The '-r' flag can be passed to change the resolution.
The '-o' flag can be passed to change the output path.

```bash
wacfrac image -r 1920 1080 -o high_def.ppm
```

The '-f' flag can be passed to change the focus or center point of the render.
Precise focus points can be very long, so it is often best to pull them
from file.
The '-z' flag can be passed to change the zoom factor of a render.
It can be parsed from scientific notation.
Putting these together is how the Mandelbrot set is explored.

```bash
wacfrac image -r 1920 1080 -f $(cat poi/big_bang) -z 2.5e10
```

The '-c' flag can be passed to change the color palette. Similar to the focus,
these are often best pulled from files or emitted from scripts. Provided with
wacfrac are several scripts which generate palettes of variable lengths.
Note that the palette must be closed by quotes.

```bash
wacfrac image -r 1920 1080 -c "$(bash palettes/fire.sh 128)"
```

The other parameters of the renderer are set to defaults which work well for most
locations. However, in some cases they may need to be manually tuned. Pass the
'--help' flag to learn about these parameters.
Read the [write up](404) to learn more about the systems these parameters operate
on.

### Video Mode

Video mode shares most of its flags with image mode. However, it necessarily
needs parameters to describe individual frames, such as the zoom factor. Instead
we must specify the first and last zoom factors with the '-a' and '-b' flags.
The '-z' flag now means to specify the zoom factor per second. This means the
view port will shrink by a factor of 'z' each second of the video.
The '--fps' flag is self descriptive.

## Building

### For Docker

All you need is Docker.
From project root run:

```bash
docker build -t wacfrac:local .
```

### Locally

Prerequisites (build)
- [Intel oneAPI DPC++/C++ Compiler](https://github.com/intel/llvm)
- NVIDIA CUDA Toolkit (optional)
- cmake >= 3.18
- boost >= 1.83.0
- gmp >= 10
- mpfr >= 6
- mpc >= 3

Additional Runtime Prerequisites
- [Intel OpenCL GPU driver (NEO)](https://github.com/intel/compute-runtime/releases/tag/26.27.39122.11) >= 26.27.39122.11
- ffmpeg
- [pastel](https://github.com/sharkdp/pastel) >= 0.12 (optional)

The Intel DPC++ compiler and OpenCL GPU driver can be installed standalone from their respective repositories or they can be installed as a part of the [Intel oneAPI Toolkit](https://www.intel.com/content/www/us/en/developer/tools/oneapi/oneapi-toolkit-download.html)

Make sure that the DPC++/C++ compiler is on your PATH before building.

If you're using the complete oneAPI toolkit:

```bash 
cmake -B build \
    -DCMAKE_C_COMPILER=icx \
    -DCMAKE_CXX_COMPILER=icpx \
    -DENABLE_CUDA_BACKEND=[ON/OFF] \
    -DSYCL_CUDA_ARCH=[YOUR ARCH]
```

If you build the DPC++/C++ compiler standalone:

```bash
cmake -B build \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DENABLE_CUDA_BACKEND=[ON/OFF] \
    -DSYCL_CUDA_ARCH=[YOUR ARCH]
```

```bash 
cmake --build build --parallel
```

## License

See [LICENSE](LICENSE.txt).
