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
    template <ComplexConcept T>
    auto render_direct(const Viewport&, unsigned max_n) -> std::span<Pixel>;
    template <ComplexConcept T>
    auto render_perturbed(const Viewport&, unsigned max_n) -> std::span<Pixel>;
    void copy_references(const ReferenceSet&);
    void reserve_references(std::size_t n);

    private:
    class Impl;
    std::unique_ptr<Impl> _pimpl;
};

} // namespace wacfrac
