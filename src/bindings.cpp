#include <nanobind/nanobind.h>
#include <nanobind/stl/complex.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/string_view.h>
#include <wacfrac/wacfrac.hpp>

namespace wf = wacfrac;
namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(wacfracpy, m) {
    nb::class_<wf::viewport>(m, "viewport")
        .def(nb::init<>())
        .def("__init__", [](wf::viewport* self, std::complex<double> mn, std::complex<double> mx) {
            new (self) wf::viewport{
                wf::multi_complex(mn.real(), mn.imag()),
                wf::multi_complex(mx.real(), mx.imag())
            };
        }, "min"_a, "max"_a)
        .def_rw("min", &wf::viewport::min)
        .def_rw("max", &wf::viewport::max)
        .def("at_zoom", &wf::viewport::at_zoom, "focus"_a, "factor"_a);

    nb::class_<wf::plot>(m, "plot")
        .def(nb::init<>())
        .def(nb::init<std::size_t, std::size_t, wf::viewport>())
        .def_rw("width",  &wf::plot::width)
        .def_rw("height", &wf::plot::height)
        .def_rw("limits", &wf::plot::limits);

    m.def("save_plot",  static_cast<bool(*)(std::string_view, const wf::plot&)>(&wf::save_plot),  "filename"_a, "plot"_a);
    m.def("save_plots", &wf::save_plots, "filename"_a, "plots"_a);
}
