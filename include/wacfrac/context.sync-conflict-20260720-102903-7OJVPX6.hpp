#pragma once
#include "wacfrac/macros.hpp"
#include "wacfrac/buffer.hpp"
#include "wacfrac/log.hpp"
#include <cstddef>
#include <cuda_runtime.h>
#include <cstring>
#include <memory>
#include <ranges>
#include <algorithm>
#include <execution>
#include <utility>

#if defined(__CUDACC__)
#include <cuda/std/span>
#endif

namespace wacfrac {

template <typename Derived>
class Context {
    private:
    template<typename T>
    using Deallocator = decltype(std::declval<Derived&>().template deallocator<T>());

    template<typename T>
    auto alloc() -> T* {
        logging::debug("Allocating {} bytes", sizeof(T));
        return static_cast<Derived&>(*this).template alloc<T>();
    }
    template<typename T>
    auto alloc(std::size_t count) -> T* {
        logging::debug("Allocating {} bytes", sizeof(T)*count);
        return static_cast<Derived&>(*this).template alloc<T>(count);
    }
    template<typename T>
    auto deallocator() -> Deallocator<T> {
        return static_cast<Derived&>(*this).template deallocator<T>();
    }

    public:
    template<typename T>
    using Buffer = Buffer<T, Deallocator<T>>; 
    template<typename T>
    using Pointer = std::unique_ptr<T, Deallocator<T>>;

    template<typename T>
    auto make_buffer(std::size_t count) {
        return Buffer<T>(alloc<T>(count), count, deallocator<T>());
    }
    template<typename T>
    auto make_buffer(WF_STD::span<T> src) {
        auto buf {Buffer<T>(alloc<T>(src.size()), src.size(), deallocator<T>())};
        Context<Derived>::memcpy<T>(buf.as_span(), src);
        return buf;
    }
    template<typename T>
    auto make_buffer(WF_STD::span<const T> src) {
        auto buf {Buffer<T>(alloc<T>(src.size()), src.size(), deallocator<T>())};
        Context<Derived>::memcpy<T>(buf.as_span(), src);
        return buf;
    }
    template<typename T>
    auto make_pointer() {
        return Pointer<T>(alloc<T>(), deallocator<T>());
    }
    template<typename T>
    auto memcpy(WF_STD::span<T> dst, WF_STD::span<const T> src) -> void {
        return static_cast<Derived&>(*this).memcpy(dst, src);
    }
    template <typename F>
    void parallel_for(std::size_t count, F&& func) const {
        return static_cast<Derived&>(*this).parallel_for(count, std::forward(func));
    }
};

struct Host : public Context<Host> {
    friend Context<Host>;

    private:
    template<typename T>
    auto alloc() -> T* {
       return new T;
    }
    template<typename T>
    auto alloc(std::size_t count) -> T* {
       return new T[count]();
    }
    template<typename T>
    auto deallocator() -> std::default_delete<T> {
        return {};
    }

    public:
    template<typename T>
    auto memcpy(WF_STD::span<T> dst, WF_STD::span<const T> src) -> void {
        // TODO: Assert sizes are the same
        std::memcpy(dst.data(), src.data(), dst.size() * sizeof(T));
    }
    template <typename F>
    void parallel_for(std::size_t count, F&& func) const {
        auto range {std::ranges::iota_view(std::size_t{0}, count)};
        std::for_each(
            std::execution::par_unseq,
            range.begin(),
            range.end(),
            func);
    }
};

#if defined(__CUDACC__)
template <typename F>
__global__ void executeLambda(std::size_t count, F func) {
    auto idx {blockIdx.x * blockDim.x + threadIdx.x};
    if (idx < count) {
        func(idx); 
    }
}
#endif

class Device : public Context<Device> {
    friend Context<Device>;
    
    static constexpr auto ThreadsPerBlock {256};

    // TODO: Support multiple GPUs by specifying device in constructor
    
    static auto count() -> int {
        auto device_count {0};
        (void) cudaGetDeviceCount(&device_count);
        return device_count;
    }

    template<typename T>
    auto alloc() -> T* {
        void* raw;
        auto err {cudaMallocManaged(&raw, sizeof(T))};
        if (err != cudaSuccess) {
            logging::error("CUDA error: {}", cudaGetErrorString(err));
            return nullptr;
        }
        return static_cast<T*>(raw);
    }
    template<typename T>
    auto alloc(std::size_t count) -> T* {
        void* raw;
        auto err {cudaMallocManaged(&raw, sizeof(T) * count)};
        if (err != cudaSuccess) {
            logging::error("CUDA error: {}", cudaGetErrorString(err));
            return nullptr;
        }
        return static_cast<T*>(raw);
    }
    template<typename T>
    struct Deallocator {
        auto operator()(T* ptr) -> void {
            cudaFree(ptr);
        }
    };
    template<typename T>
    auto deallocator() -> Deallocator<T> {
        return {};
    }

    public:
    template<typename T>
    auto memcpy(WF_STD::span<T> dst, WF_STD::span<const T> src) -> void {
        // TODO: Assert sizes are the same
        cudaMemcpy(dst.data(), src.data(), dst.size() * sizeof(T), cudaMemcpyDefault);
        // TODO: Catch errors
    }
#if defined(__CUDACC__)
    template <typename F>
    void parallel_for(std::size_t count, F&& func) const {
        auto numBlocks {(count + ThreadsPerBlock - 1) / ThreadsPerBlock};
        executeLambda<<<numBlocks, ThreadsPerBlock>>>(count, func);

        auto err {cudaDeviceSynchronize()}; // TODO: Refactor into free func
        if (err != cudaSuccess) {
            logging::error("CUDA error: {}", cudaGetErrorString(err));
        }
        err = cudaGetLastError();
        if (err != cudaSuccess) {
            logging::error("CUDA error: {}", cudaGetErrorString(err));
        }
    }
#else
    template <typename F>
    void parallel_for(std::size_t count, F&& func) const;
#endif
};

}
