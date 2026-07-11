#pragma once

#include "wacfrac/complex_adapter.hpp"
#include "wacfrac/types.hpp"
#include <cccl/cuda/__container/buffer.h>
#include <cccl/cuda/__device/device_ref.h>
#include <cccl/cuda/__stream/stream.h>
#include <cuda/devices>
#include <cuda/stream>
#include <cuda/memory_pool>
#include <cuda/buffer>
#include <type_traits>

namespace wacfrac {

struct DeviceReferenceSet {
    cuda::device_ref device;
    cuda::stream_ref stream;
    cuda::device_buffer<SingleComplex> float_ref;
    cuda::device_buffer<DoubleComplex> double_ref;
    cuda::device_buffer<DoubleExpComplex> dexp_ref;

    DeviceReferenceSet(cuda::device_ref device, cuda::stream_ref stream) // TODO: Copy from ReferenceSet
        : device(device)
        , stream(stream)
        , float_ref {stream, cuda::device_default_memory_pool(device)}
        , double_ref {stream, cuda::device_default_memory_pool(device)}
        , dexp_ref {stream, cuda::device_default_memory_pool(device)}
    {}

    template <typename T>
    auto select() const -> const auto& {
        if constexpr (std::is_same_v<T, SingleComplex>)
            return float_ref;
        else if constexpr (std::is_same_v<T, DoubleComplex>)
            return double_ref;
        else
            return dexp_ref;
    }

    template <typename T>
    auto select() -> auto& {
        if constexpr (std::is_same_v<T, SingleComplex>)
            return float_ref;
        else if constexpr (std::is_same_v<T, DoubleComplex>)
            return double_ref;
        else
            return dexp_ref;
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
        reserve<SingleComplex>(size);
        reserve<DoubleComplex>(size);
        reserve<DoubleExpComplex>(size);
    }
};

} // namespace wacfrac
