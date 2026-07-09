#include <climits>
#include <cstddef>
#include <vector>
#include "wacfrac/gpu/types.cuh"
#include "wacfrac/gpu/host_interface.hpp"
#include "wacfrac/gpu/rendering.cuh"
#include "wacfrac/complex_concept.hpp"
#include "wacfrac/log.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/types.hpp"
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
    ReferenceSet host_references;
    gpu::ReferenceSet device_references;
    Resolution resolution;

    Impl(int device_id, const Resolution& resolution, const std::vector<Pixel>& palette, std::size_t ref_capacity)
        : device{0}
        , stream{device}
        , palette{cuda::make_buffer<Pixel>(stream, cuda::device_default_memory_pool(device), palette.begin(), palette.end())}
        , pixels{cuda::make_buffer<Pixel>(stream, cuda::pinned_default_memory_pool(), resolution.area(), cuda::no_init)}
        , device_references{device, stream}
        , resolution{resolution} {
        host_references.reserve(ref_capacity);
        wacfrac::logging::debug("Constructed GpuRender::Impl");
    } 

    template <Complex T>
    inline auto render_direct(const Viewport& view, unsigned max_n) -> std::span<Pixel> {
        using CT = ComplexValueTypeT<T>;
        MultiComplex start_mc {view.center - view.dimensions / 2.0};
        T start {
            static_cast<CT>(start_mc.real()),
            static_cast<CT>(start_mc.imag())
        };
        T delta {
            static_cast<CT>(view.dimensions.real()) / static_cast<CT>(resolution.width),
            static_cast<CT>(view.dimensions.imag()) / static_cast<CT>(resolution.height)
        };
        wacfrac::logging::debug("Pixel delta: {} + i{}", delta.real(), delta.imag());
        
        constexpr auto threads_per_block {256};
        auto config {cuda::distribute<threads_per_block>(resolution.area())};
        wacfrac::logging::debug("Launching direct render kernel");
        cuda::launch(stream, config, gpu::render_direct<T, decltype(config)>,
                    pixels, resolution.width, start, delta,
                    max_n, palette);
        stream.sync();
        wacfrac::logging::debug("Finished with render");
        return pixels;
    }

    template <Complex T>
    inline auto render_perturbed(const Viewport& view, unsigned max_n) -> std::span<Pixel> {
        using CT = ComplexValueTypeT<T>;
        T ref_c {to_complex<T>(view.center)};
        MultiComplex corner_mc {view.center - view.dimensions / 2.0};
        T start {to_complex<T>(corner_mc) - ref_c};
        T delta {
            static_cast<CT>(view.dimensions.real()) / static_cast<CT>(resolution.width),
            static_cast<CT>(view.dimensions.imag()) / static_cast<CT>(resolution.height)
        };
        wacfrac::logging::debug("Pixel delta: {} + i{}", delta.real(), delta.imag());
        
        constexpr auto threads_per_block {256};
        auto config {cuda::distribute<threads_per_block>(resolution.area())};
        wacfrac::logging::debug("Launching perturbed render kernel");
        cuda::launch(stream, config, gpu::render_perturbed<T, decltype(config)>,
                    pixels, resolution.width, start, delta, device_references.select<T>(),
                    max_n, palette);
        stream.sync();
        wacfrac::logging::debug("Finished with render");
        return pixels;
    }
    void reserve_references(std::size_t n) {
        host_references.reserve(n);
        device_references.reserve(n);
    }

    template <Complex T>
    inline void copy_reference(std::span<const T> reference) {
        auto& host_ref {host_references.select<T>()};
        auto& device_ref {device_references.select<T>()};
        host_ref.insert(host_ref.end(), reference.begin(), reference.end());
        device_ref = cuda::make_buffer<cuda::std::complex<double>>(
            stream, cuda::device_default_memory_pool(device),
            host_ref.begin(), host_ref.end());
    }

    inline void copy_references(const ReferenceSet& references) {
        copy_reference<std::complex<double>>(references.select<std::complex<double>>());
    }
};

GpuRenderer::GpuRenderer(int device_id, const Resolution& resolution, const std::vector<Pixel>& palette, std::size_t reference_capacity)
    : _pimpl(std::make_unique<Impl>(device_id, resolution, palette, reference_capacity)) {}
GpuRenderer::~GpuRenderer() = default;

template <Complex T>
auto GpuRenderer::render_direct(const Viewport& view, unsigned max_n) -> std::span<Pixel> {
    return _pimpl->render_direct<T>(view, max_n);
}
template
auto GpuRenderer::render_direct<cuda::std::complex<double>>(const Viewport& view, unsigned max_n) -> std::span<Pixel>;

template <Complex T>
auto GpuRenderer::render_perturbed(const Viewport& view, unsigned max_n) -> std::span<Pixel> {
    return _pimpl->render_perturbed<T>(view, max_n);
}
template
auto GpuRenderer::render_perturbed<cuda::std::complex<double>>(const Viewport& view, unsigned max_n) -> std::span<Pixel>;

void GpuRenderer::copy_references(const ReferenceSet& references) {
    _pimpl->copy_references(references);
}

void GpuRenderer::reserve_references(std::size_t n) {
    _pimpl->reserve_references(n);
}

auto GpuRenderer::device_count() -> int {
    return static_cast<int>(cuda::devices.size());
}

} // namespace wacfrac
