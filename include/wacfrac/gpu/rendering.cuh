#pragma once

#include "wacfrac/complex_concept.hpp"
#include "wacfrac/color.hpp"
#include "wacfrac/orbit.hpp"
#include <cuda/__functional/call_or.h>
#include <cuda/buffer> 
#include <cuda/devices>
#include <cuda/launch>
#include <cuda/memory_pool>
#include <cuda/std/span>
#include <cuda/stream>
#include <cstdio>
#include <cstddef>

namespace wacfrac {

namespace gpu {

constexpr auto ESCAPE_RADIUS {4.0};

template <Complex T>
__inline__ __device__
auto sample_c_value(std::size_t idx,
                    std::size_t row_width,
                    T start,
                    T delta) -> T {
    auto x {idx % row_width};
    auto y {idx / row_width};
    return T{
        start.real() + delta.real() * x,
        start.imag() + delta.imag() * y
    };
}

template <Complex T, typename Config>
__global__
void render_direct(Config config,
            cuda::std::span<Pixel> pixels,
            std::size_t row_width,
            T start,
            T delta,
            unsigned max_iterations,
            cuda::std::span<const Pixel> palette) {
    auto tid {cuda::gpu_thread.rank(cuda::grid, config)};
    if (tid < pixels.size()) {

        auto [z, n] {escape(
            sample_c_value(
                tid,
                row_width,
                start,
                delta),
            max_iterations,
            ESCAPE_RADIUS)};
        pixels[tid] = colorize_discrete(n, max_iterations, palette);
    }
}

template <Complex T, typename Config>
__global__
void render_perturbed(Config config,
            cuda::std::span<Pixel> pixels,
            std::size_t row_width,
            T start,
            T delta,
            cuda::std::span<const T> reference,
            unsigned max_iterations,
            cuda::std::span<const Pixel> palette) {
    auto tid {cuda::gpu_thread.rank(cuda::grid, config)};
    if (tid == 0) {
        std::printf("ref size %d\n", reference.size());
    }
    if (tid < pixels.size()) {
        auto [z, n] {escape_perturbed(
            sample_c_value(
                tid,
                row_width,
                start,
                delta),
            reference,
            max_iterations,
            ESCAPE_RADIUS)};
        pixels[tid] = colorize_continuous(z, n, max_iterations, palette);
    }
}

} // namespace gpu

} // namespace gpu
