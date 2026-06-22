// https://philthompson.me/2023/Faster-Mandelbrot-Set-Rendering-with-BLA-Bivariate-Linear-Approximation.html
#include "wacfrac/types.hpp"
#include <wacfrac/bla.tpp>

template class wacfrac::bivariate_linear_approximator<std::complex<double>>;
template class wacfrac::bivariate_linear_approximator<std::complex<long double>>;
template class wacfrac::bivariate_linear_approximator<wacfrac::doubleexp_complex>;
