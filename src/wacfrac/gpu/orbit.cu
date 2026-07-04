#include "wacfrac/gpu/orbit.cuh"

namespace wacfrac::gpu {

template <Complex T>
__device__
auto escaped(T z, float escape_radius) -> bool
{
    return false;
}

template <Complex T>
__device__
auto escape(T c, std::size_t max_n, float escape_radius) -> std::size_t
{
    return 0;
}

template __device__ auto escaped(cuda::std::complex<float>, float) -> bool;
template __device__ auto escaped(cuda::std::complex<double>, float) -> bool;
template __device__ auto escape(cuda::std::complex<float>, std::size_t, float) -> std::size_t;
template __device__ auto escape(cuda::std::complex<double>, std::size_t, float) -> std::size_t;

} // namespace wacfrac::gpu
