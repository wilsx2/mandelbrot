#pragma once
#include "wacfrac/color.hpp"
#include "wacfrac/resolution.hpp"

namespace wacfrac::gpu {

class Renderer {
    public:
    Renderer() = default;
    void set_dimensions(const Resolution&);
    void set_palette(const std::vector<Pixel>&);
    auto render() -> const std::vector<Pixel>&;

    private:
    class Impl;
    Impl* _pimpl;
};

} // namespace wacfrac::gpu
