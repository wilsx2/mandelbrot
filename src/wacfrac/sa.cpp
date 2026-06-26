// https://web.archive.org/web/20220125200420/http://www.science.eclipse.co.uk/sft_maths.pdf
#include "wacfrac/types.hpp"
#include <wacfrac/sa.tpp>

template class wacfrac::SeriesApproximator<std::complex<double>>;
template class wacfrac::SeriesApproximator<std::complex<long double>>;
template class wacfrac::SeriesApproximator<wacfrac::DoubleExpComplex>;
