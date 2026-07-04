#include "wacfrac/gpu/color.cuh"

namespace wacfrac::gpu {

__device__
auto colorize_discrete(cuda::std::span<const Pixel> palette,
                       std::size_t max_n,
                       cuda::std::span<const std::size_t> n) -> Pixel
{
    return Pixel{0, 0, 0};
}

} // namespace wacfrac::gpu
