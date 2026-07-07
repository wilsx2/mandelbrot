#pragma once
#include "wacfrac/color.hpp"
#include "wacfrac/resolution.hpp"
#include "wacfrac/viewport.hpp"
#include <memory>

namespace wacfrac {

class GpuRenderer {
    public:
    static auto device_count() -> int;
    GpuRenderer(int device_id, const Resolution& resolution, const std::vector<Pixel>& palette, std::size_t reference_capacity = 0);
    ~GpuRenderer();
    template <Complex T>
    auto render_direct(const Viewport&, std::size_t max_n, double escape_radius = 4.0, bool discrete = false) -> std::span<Pixel>;
    template <Complex T>
    auto render_perturbed(const Viewport&, std::size_t max_n, double escape_radius = 4.0, bool discrete = false) -> std::span<Pixel>;
    template <Complex T>
    void copy_reference(const std::span<T>& reference);
    void reserve_reference(std::size_t n);

    private:
    class Impl;
    std::unique_ptr<Impl> _pimpl;
};

} // namespace wacfrac
