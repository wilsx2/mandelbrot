#include "wacfrac/gpu/rendering.cuh"

namespace wacfrac::gpu {

template <Complex T>
__device__
auto sample_c_value(std::size_t idx,
                    Resolution res,
                    T delta,
                    float escape_radius) -> T
{
    return T{};
}

template <Complex T>
__global__
void render(Pixel* pixels,
            std::size_t width,
            std::size_t height,
            T start,
            T delta,
            float escape_radius,
            std::size_t max_iterations,
            cuda::std::span<const Pixel> palette)
{
}

template __device__ auto sample_c_value(std::size_t, Resolution, cuda::std::complex<float>, float) -> cuda::std::complex<float>;
template __device__ auto sample_c_value(std::size_t, Resolution, cuda::std::complex<double>, float) -> cuda::std::complex<double>;

} // namespace wacfrac::gpu
