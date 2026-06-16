#include <nanobind/nanobind.h>
#include <nanobind/stl/complex.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/ndarray.h>
#include <wacfrac/wacfrac.hpp>

namespace wf = wacfrac;
namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(wacfracpy, m) {
    // pixel
    nb::class_<wf::rgb>(m, "pixel")
        .def(nb::init<uint8_t, uint8_t, uint8_t>(), "r"_a, "g"_a, "b"_a)
        .def_rw("r", &wf::rgb::r)
        .def_rw("g", &wf::rgb::g)
        .def_rw("b", &wf::rgb::b);

    // resolution
    nb::class_<wf::resolution>(m, "resolution")
        .def(nb::init<std::size_t, std::size_t>(), "width"_a, "height"_a)
        .def_rw("width",  &wf::resolution::width)
        .def_rw("height", &wf::resolution::height)
        .def("area", &wf::resolution::area);

    // viewport
    nb::class_<wf::viewport>(m, "viewport")
        .def(nb::init<>())
        .def("__init__", [](wf::viewport* self, std::complex<double> center, std::complex<double> dimensions) {
            new (self) wf::viewport{
                wf::multi_complex(center.real(), center.imag()),
                wf::multi_complex(dimensions.real(), dimensions.imag())
            };
        }, "center"_a, "dimensions"_a)
        .def_rw("center", &wf::viewport::center)
        .def_rw("dimensions", &wf::viewport::dimensions)
        .def("precision", &wf::viewport::precision, "value"_a)
        .def("zoomed", [](const wf::viewport& self, double factor) {
            return self.zoomed(wf::multi_float(factor));
         }, "factor"_a)
        .def("zoomed", [](const wf::viewport& self, std::string factor) {
            return self.zoomed(wf::multi_float(factor));
         }, "factor"_a)
        .def("sample", [](const wf::viewport& self, std::size_t x, std::size_t y,
                          std::size_t width, std::size_t height) {
            auto c = self.sample(x, y, width, height);
            return std::complex<double>(
                static_cast<double>(c.real()),
                static_cast<double>(c.imag())
            );
        }, "x"_a, "y"_a, "width"_a, "height"_a);

    // // plot
    // nb::class_<wf::plot>(m, "plot")
    //     .def(nb::init<>())
    //     .def("__init__", [](wf::plot* self, wf::resolution res, wf::viewport view, std::size_t max_iterations) {
    //         new (self) wf::plot{ res, view, max_iterations };
    //     }, "res"_a, "view"_a, "max_iterations"_a)
    //     .def_rw("res",            &wf::plot::res)
    //     .def_rw("view",           &wf::plot::view)
    //     .def_rw("max_iterations", &wf::plot::max_iterations);

    // // render functions - return pixel buffer as list
    // m.def("render_directly", [](const wf::plot& p) -> std::vector<wf::pixel> {
    //     std::vector<wf::pixel> buffer(p.res.area());
    //     wf::render_directly(p, buffer);
    //     return buffer;
    // }, "plot"_a);

    // m.def("render_perturbed", [](const wf::plot& p) -> std::vector<wf::pixel> {
    //     std::vector<wf::pixel> buffer(p.res.area());
    //     wf::render_perturbed(p, buffer);
    //     return buffer;
    // }, "plot"_a);

    // io functions
    // m.def("save_to_ppm",
    //     static_cast<bool(*)(std::string_view, const wf::plot&)>(&wf::save_to_ppm),
    //     "filename"_a, "plot"_a);
    // m.def("save_to_ppm",
    //     static_cast<bool(*)(std::string_view, const std::vector<wf::plot>&)>(&wf::save_to_ppm),
    //     "filename"_a, "plots"_a);

    // color utilities
    m.def("hsv_to_rgb", &wf::hsv_to_rgb, "h"_a, "s"_a, "v"_a);
    // lch_to_rgb is declared but not yet implemented (TODO in color.hpp)

    // constants
    m.attr("FULL_SET") = wf::FULL_SET;

    nb::module_ res_mod = m.def_submodule("video_resolution", "Standard video resolutions");
    res_mod.attr("SD360p")  = wf::video_resolution::SD360p;
    res_mod.attr("SD480p")  = wf::video_resolution::SD480p;
    res_mod.attr("HD720p")  = wf::video_resolution::HD720p;
    res_mod.attr("HD1080p") = wf::video_resolution::HD1080p;
    res_mod.attr("UHD4K")   = wf::video_resolution::UHD4K;

    nb::module_ poi_mod = m.def_submodule("poi", "Points of interest");
    poi_mod.attr("MISIUREWICZ")         = [](){ auto c = wf::poi::MISIUREWICZ;         return std::complex<double>(static_cast<double>(c.real()), static_cast<double>(c.imag())); }();
    poi_mod.attr("BIG_BANG")         = [](){ auto c = wf::poi::BIG_BANG;         return std::complex<double>(static_cast<double>(c.real()), static_cast<double>(c.imag())); }();
}
