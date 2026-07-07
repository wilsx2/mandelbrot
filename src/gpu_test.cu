#include "wacfrac/color.hpp"
#include "wacfrac/viewport.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/gpu/rendering.cuh"
#include <cuda/__functional/call_or.h>
#include <cuda/buffer> 
#include <cuda/devices>
#include <cuda/launch>
#include <cuda/memory_pool>
#include <cuda/std/span>
#include <cuda/std/complex>
#include <cuda/stream>
#include <iostream>

void gpu_rendering_pass(std::span<wacfrac::Pixel> pixels,
                        wacfrac::Resolution res,
                        wacfrac::Viewport view) {
    // Resource setup
    cuda::device_ref device{cuda::devices[0]};
    cuda::stream stream{device};
    auto device_pool{cuda::device_default_memory_pool(device)};
    auto host_pool{cuda::pinned_default_memory_pool()};
    auto P {cuda::make_buffer<wacfrac::Pixel>(stream, host_pool, res.area(), cuda::no_init)};

    // Kernel Call 
    auto config {cuda::distribute<256>(res.area())};
    cuda::launch(stream, config, wacfrac::gpu::render<cuda::std::complex<double>, decltype(config)>,
        P, 
        res.height,
        view.center - view.dimensions/2.0,
        {
            view.dimensions.real() / static_cast<double>(res.width),
            view.dimensions.imag() / static_cast<double>(res.height),
        },
        4.0,
        cuda::make_buffer<wacfrac::Pixel>(stream, device_pool, wacfrac::ULTRA.begin(), wacfrac::ULTRA.end()));
}
