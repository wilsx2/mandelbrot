#pragma once

#include "wacfrac/complex_adapter.hpp"
#include <cccl/cuda/__container/buffer.h>
#include <cccl/cuda/__device/device_ref.h>
#include <cccl/cuda/__stream/stream.h>
#include <cuda/devices>
#include <cuda/stream>
#include <cuda/memory_pool>
#include <cuda/buffer>
#include <type_traits>

namespace wacfrac::gpu {

struct ReferenceSet {
    cuda::device_ref device;
    cuda::stream_ref stream;
    cuda::device_buffer<wacfrac::Complex<float>> float_ref;
    cuda::device_buffer<wacfrac::Complex<double>> double_ref;

    ReferenceSet(cuda::device_ref device, cuda::stream_ref stream)
        : device(device)
        , stream(stream)
        , float_ref {stream, cuda::device_default_memory_pool(device)}
        , double_ref {stream, cuda::device_default_memory_pool(device)}
    {}

    template <typename T>
    auto select() const -> const auto& {
        if constexpr (std::is_same_v<T, wacfrac::Complex<float>>)
            return float_ref;
        else
            return double_ref;
    }

    template <typename T>
    auto select() -> auto& {
        if constexpr (std::is_same_v<T, wacfrac::Complex<float>>)
            return float_ref;
        else
            return double_ref;
    }

    template <typename T>
    void reserve(std::size_t size) {
        auto& ref {select<T>()};
        if (size > ref.size()) {
            ref = cuda::make_buffer<T>(
                stream, cuda::device_default_memory_pool(device),
                size, cuda::no_init);
        }
    }

    void reserve(std::size_t size) {
        reserve<wacfrac::Complex<float>>(size);
        reserve<wacfrac::Complex<double>>(size);
    }
};

} // namespace wacfrac::gpu
