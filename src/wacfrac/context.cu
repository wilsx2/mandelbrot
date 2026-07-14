#include "wacfrac/context.hpp"
#include "wacfrac/bla.hpp"
#include "wacfrac/log.hpp"
#include <cuda_runtime.h>

namespace wacfrac {

template<typename T>
auto Device::alloc() -> T* {
    void* raw;
    auto err {cudaMallocManaged(&raw, sizeof(T))};
    if (err != cudaSuccess) {
        logging::error("CUDA error: {}", cudaGetErrorString(err));
    }
    return static_cast<T*>(raw);
}
template<typename T>
auto Device::alloc(std::size_t count) -> T* {
    void* raw;
    auto err {cudaMallocManaged(&raw, sizeof(T) * count)};
    if (err != cudaSuccess) {
        logging::error("CUDA error: {}", cudaGetErrorString(err));
    }
    return static_cast<T*>(raw);
}
template<typename T>
auto Device::Deallocator<T>::operator()(T* ptr) -> void {
    cudaFree(ptr);
}
template<typename T>
auto Device::deallocator() -> Deallocator<T> {
    return {};
}

template auto Device::alloc<bla::ColumnInfo>(std::size_t) -> bla::ColumnInfo*;
template auto Device::alloc<bla::ColumnInfo>() -> bla::ColumnInfo*;
template auto Device::deallocator<bla::ColumnInfo>() -> Device::Deallocator<bla::ColumnInfo>;
template auto Device::Deallocator<bla::ColumnInfo>::operator()(bla::ColumnInfo*) -> void;

template auto Device::alloc<bla::Bla<Complex<double>>>(std::size_t) -> bla::Bla<Complex<double>>*;
template auto Device::alloc<bla::Bla<Complex<double>>>() -> bla::Bla<Complex<double>>*;
template auto Device::deallocator<bla::Bla<Complex<double>>>() -> Device::Deallocator<bla::Bla<Complex<double>>>;
template auto Device::Deallocator<bla::Bla<Complex<double>>>::operator()(bla::Bla<Complex<double>>*) -> void;

template auto Device::alloc<unsigned int>(std::size_t) -> unsigned int*;
template auto Device::alloc<unsigned int>() -> unsigned int*;
template auto Device::deallocator<unsigned int>() -> Device::Deallocator<unsigned int>;
template auto Device::Deallocator<unsigned int>::operator()(unsigned int*) -> void;

template auto Device::alloc<cuda::std::atomic<unsigned int>>() -> cuda::std::atomic<unsigned int>*;
template auto Device::deallocator<cuda::std::atomic<unsigned int>>() -> Device::Deallocator<cuda::std::atomic<unsigned int>>;
template auto Device::Deallocator<cuda::std::atomic<unsigned int>>::operator()(cuda::std::atomic<unsigned int>*) -> void;

template auto Device::alloc<cuda::std::atomic<bool>>() -> cuda::std::atomic<bool>*;
template auto Device::deallocator<cuda::std::atomic<bool>>() -> Device::Deallocator<cuda::std::atomic<bool>>;
template auto Device::Deallocator<cuda::std::atomic<bool>>::operator()(cuda::std::atomic<bool>*) -> void;

} // namespace wacfrac
