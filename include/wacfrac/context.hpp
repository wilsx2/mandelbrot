#pragma once
#include "wacfrac/macros.hpp"
#include "wacfrac/log.hpp"
#include <memory>
#include <ranges>
#include <algorithm>
#include <execution>
#include <functional>

#if defined(__CUDACC__)
#include <cuda/std/span>
#endif

namespace wacfrac {

template<typename T, typename Deleter = ::std::default_delete<T[]>>
class Buffer {
    private:
    std::unique_ptr<T[], Deleter> _data;
    std::size_t _size;

    public:
    Buffer() : _data(nullptr), _size(0) {}
    Buffer(T* raw, std::size_t size, Deleter del = {})
        : _data(raw, del)
        , _size(size)
    {}
    auto as_span() const { return WF_STD::span<T>(_data.get(), _size); }
    auto data() const {
        return _data.get();
    }
    auto size() const {
        return _size;
    }
    operator WF_STD::span<T>() const { return {_data.get(), _size}; }
    operator WF_STD::span<const T>() const { return {_data.get(), _size}; }

    friend void swap(Buffer& lhs, Buffer& rhs) {
        WF_STD::swap(lhs._data, rhs._data);
        WF_STD::swap(lhs._size, rhs._size);
    }
};


template <typename Derived>
class Context {
    private:
    template<typename T>
    using Deallocator = decltype(std::declval<Derived&>().template deallocator<T>());

    template<typename T>
    auto alloc() -> T* {
        // TODO: Log
        return static_cast<Derived&>(*this).template alloc<T>();
    }
    template<typename T>
    auto alloc(std::size_t count) -> T* {
        // TODO: Log
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
    auto make_pointer() {
        return Pointer<T>(alloc<T>(), deallocator<T>());
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

class Device : public Context<Device> {
    friend Context<Device>;
    
    static constexpr auto ThreadsPerBlock {256};

    // TODO: Support multiple GPUs by specifying device in constructor

    template<typename T>
    auto alloc() -> T*;
    template<typename T>
    auto alloc(std::size_t count) -> T*;
    template<typename T>
    struct Deallocator {
        auto operator()(T* ptr) -> void;
    };
    template<typename T>
    auto deallocator() -> Deallocator<T>;

    public:
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
};
#endif

}
