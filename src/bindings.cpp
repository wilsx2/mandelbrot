#include <nanobind/nanobind.h>
#include <nanobind/stl/complex.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/string_view.h>
#include <mplot/mplot.hpp>

namespace mp = mplot;
namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(mplotpy, m) {
    nb::class_<mp::axis_limits>(m, "axis_limits")
        .def(nb::init<>())
        .def(nb::init<std::complex<double>, std::complex<double>>())
        .def_rw("min", &mp::axis_limits::min)
        .def_rw("max", &mp::axis_limits::max)
        .def("at_zoom", &mp::axis_limits::at_zoom, "focus"_a, "factor"_a);

    nb::class_<mp::plot>(m, "plot")
        .def(nb::init<>())
        .def(nb::init<std::size_t, std::size_t, mp::axis_limits>())
        .def_rw("width",  &mp::plot::width)
        .def_rw("height", &mp::plot::height)
        .def_rw("limits", &mp::plot::limits);

    m.def("save_plot", static_cast<bool(*)(std::string_view, const mp::plot&)>(&mp::save_plot), "filename"_a, "plot"_a);
    m.def("save_plots", &mp::save_plots, "filename"_a, "plots"_a);
}
