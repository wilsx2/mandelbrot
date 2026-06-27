#include <wacfrac/orbit.tpp>

namespace wacfrac
{

template auto rebase_reference<std::complex<double>>(const std::vector<std::complex<double>>& ref, std::size_t ref_n, std::complex<double> dz) -> std::tuple<std::size_t, std::complex<double>, std::complex<double>>;
template auto compute_next_perturbation<std::complex<double>>(const std::vector<std::complex<double>>& ref, std::size_t ref_n, std::complex<double> dc, std::complex<double> dz) -> std::tuple<std::size_t, std::complex<double>, std::complex<double>>;
template auto escape_perturbed<std::complex<double>>(const std::vector<std::complex<double>>& ref, std::complex<double> dc, std::size_t max_n, double escape_radius, std::complex<double> dz, std::size_t n) -> std::pair<std::complex<double>, std::size_t>;
template auto compute_reference<std::complex<double>>(MultiComplex c, std::size_t max_n, double escape_radius) -> std::vector<std::complex<double>>;
template auto compute_reference_mt<std::complex<double>>(MultiComplex c, std::size_t max_n, double escape_radius) -> std::vector<std::complex<double>>;

template auto rebase_reference<std::complex<long double>>(const std::vector<std::complex<long double>>& ref, std::size_t ref_n, std::complex<long double> dz) -> std::tuple<std::size_t, std::complex<long double>, std::complex<long double>>;
template auto compute_next_perturbation<std::complex<long double>>(const std::vector<std::complex<long double>>& ref, std::size_t ref_n, std::complex<long double> dc, std::complex<long double> dz) -> std::tuple<std::size_t, std::complex<long double>, std::complex<long double>>;
template auto escape_perturbed<std::complex<long double>>(const std::vector<std::complex<long double>>& ref, std::complex<long double> dc, std::size_t max_n, double escape_radius, std::complex<long double> dz, std::size_t n) -> std::pair<std::complex<long double>, std::size_t>;
template auto compute_reference<std::complex<long double>>(MultiComplex c, std::size_t max_n, double escape_radius) -> std::vector<std::complex<long double>>;
template auto compute_reference_mt<std::complex<long double>>(MultiComplex c, std::size_t max_n, double escape_radius) -> std::vector<std::complex<long double>>;

template auto rebase_reference<DoubleExpComplex>(const std::vector<DoubleExpComplex>& ref, std::size_t ref_n, DoubleExpComplex dz) -> std::tuple<std::size_t, DoubleExpComplex, DoubleExpComplex>;
template auto compute_next_perturbation<DoubleExpComplex>(const std::vector<DoubleExpComplex>& ref, std::size_t ref_n, DoubleExpComplex dc, DoubleExpComplex dz) -> std::tuple<std::size_t, DoubleExpComplex, DoubleExpComplex>;
template auto escape_perturbed<DoubleExpComplex>(const std::vector<DoubleExpComplex>& ref, DoubleExpComplex dc, std::size_t max_n, double escape_radius, DoubleExpComplex dz, std::size_t n) -> std::pair<DoubleExpComplex, std::size_t>;
template auto compute_reference<DoubleExpComplex>(MultiComplex c, std::size_t max_n, double escape_radius) -> std::vector<DoubleExpComplex>;
template auto compute_reference_mt<DoubleExpComplex>(MultiComplex c, std::size_t max_n, double escape_radius) -> std::vector<DoubleExpComplex>;
}   // namespace wacfrac
