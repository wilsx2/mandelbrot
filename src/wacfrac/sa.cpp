// https://web.archive.org/web/20220125200420/http://www.science.eclipse.co.uk/sft_maths.pdf
#include "wacfrac/types.hpp"
#include <wacfrac/sa.tpp>

template class wacfrac::series_approximator<std::complex<double>>;
template class wacfrac::series_approximator<std::complex<long double>>;
template class wacfrac::series_approximator<wacfrac::doubleexp_complex>;
