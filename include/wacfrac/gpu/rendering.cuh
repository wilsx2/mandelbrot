#pragma once

#include <cstddef>
#include "wacfrac/color.hpp"
#include "wacfrac/gpu/color.cuh"
#include "wacfrac/gpu/orbit.cuh"
#include "wacfrac/complex_concept.hpp"
#include <cuda/__functional/call_or.h>
#include <cuda/buffer> 
#include <cuda/devices>
#include <cuda/launch>
#include <cuda/memory_pool>
#include <cuda/std/span>
#include <cuda/std/complex>
#include <cuda/stream>
#include <cstdio>

namespace wacfrac {

namespace gpu {

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
            float escape_radius,
            std::size_t max_iterations,
            bool discrete_coloring,
            cuda::std::span<const Pixel> palette) {
    auto tid {cuda::gpu_thread.rank(cuda::grid, config)};
    if (tid < pixels.size()) {

        auto [z, n] {escape_direct(
            sample_c_value(
                tid,
                row_width,
                start,
                delta),
            max_iterations,
            escape_radius)};
        pixels[tid] = discrete_coloring
            ? colorize_discrete(n, max_iterations, palette)
            : colorize_continuous(z, n, max_iterations, palette);
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
            float escape_radius,
            std::size_t max_iterations,
            bool discrete_coloring,
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
            escape_radius)};
        pixels[tid] = discrete_coloring
            ? colorize_discrete(n, max_iterations, palette)
            : colorize_continuous(z, n, max_iterations, palette);
    }
}

} // namespace gpu

} // namespace gpu
