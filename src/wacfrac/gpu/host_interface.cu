#include <cstddef>
#include "wacfrac/gpu/host_interface.hpp"
#include "wacfrac/complex_concept.hpp"
#include "wacfrac/gpu/rendering.cuh"
#include "wacfrac/log.hpp"
#include "wacfrac/resolution.hpp"
#include <cccl/cuda/__container/buffer.h>
#include <cccl/cuda/__device/device_ref.h>
#include <cccl/cuda/__memory_pool/device_memory_pool.h>
#include <cccl/cuda/__memory_pool/pinned_memory_pool.h>
#include <cccl/cuda/__memory_resource/shared_resource.h>
#include <cccl/cuda/__utility/no_init.h>
#include <complex>
#include <cuda/buffer>
#include <cuda/devices>
#include <cuda/memory_pool>
#include <cuda/stream>

namespace wacfrac {

struct GpuRenderer::Impl {
    cuda::device_ref device;
    cuda::stream stream;
    cuda::device_buffer<Pixel> palette;
    cuda::host_buffer<Pixel> pixels;
    Resolution resolution;

    Impl(int device_id, const Resolution& resolution, const std::vector<Pixel>& palette)
        : device{0}
        , stream{device}
        , palette{cuda::make_buffer<Pixel>(stream, cuda::device_default_memory_pool(device), palette.begin(), palette.end())}
        , pixels{cuda::make_buffer<Pixel>(stream, cuda::pinned_default_memory_pool(), resolution.area(), cuda::no_init)}
        , resolution{resolution} {
        wacfrac::logging::debug("Constructed GpuRender::Impl");
    } 
    template <Complex T>
    inline auto render(const Viewport& view, std::size_t max_n, double escape_radius, bool discrete) -> std::span<Pixel> {
        using CT = ComplexValueTypeT<T>;
        T start {view.center - view.dimensions / 2.0};
        T delta { // NOTE: Not using to_real casts defined in wacfrac/types.hpp
            static_cast<CT>(view.dimensions.real()) / static_cast<CT>(resolution.width),
            static_cast<CT>(view.dimensions.real()) / static_cast<CT>(resolution.height)
        };
        wacfrac::logging::debug("Pixel delta: {} + i{}", delta.real(), delta.imag());
        
        constexpr auto threads_per_block {256};
        auto config {cuda::distribute<threads_per_block>(resolution.area())};
        wacfrac::logging::debug("Launching render kernel");
        cuda::launch(stream, config, gpu::render<T, decltype(config)>,
                    pixels, resolution.width, start, delta,
                    escape_radius, max_n, palette);
        (void) discrete;
        stream.sync();
        wacfrac::logging::debug("Finished with render");
        return pixels;
    }
};

GpuRenderer::GpuRenderer(int device_id, const Resolution& resolution, const std::vector<Pixel>& palette)
    : _pimpl(std::make_unique<Impl>(device_id, resolution, palette)) {}
GpuRenderer::~GpuRenderer() = default;

template <Complex T>
auto GpuRenderer::render(const Viewport& view, std::size_t max_n, double escape_radius, bool discrete) -> std::span<Pixel> {
    return _pimpl->render<T>(view, max_n, escape_radius, discrete);
}
template
auto GpuRenderer::render<std::complex<double>>(const Viewport& view, std::size_t max_n, double escape_radius, bool discrete) -> std::span<Pixel>;


auto GpuRenderer::device_count() -> int {
    return static_cast<int>(cuda::devices.size());
}

} // namespace wacfrac
