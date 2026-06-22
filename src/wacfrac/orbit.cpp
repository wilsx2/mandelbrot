#include <wacfrac/orbit.tpp>

namespace wacfrac
{

template auto rebase_reference<std::complex<double>>(const std::vector<std::complex<double>>& ref, std::size_t ref_n, std::complex<double> dz) -> std::tuple<std::size_t, std::complex<double>, std::complex<double>>;
template auto compute_next_perturbation<std::complex<double>>(const std::vector<std::complex<double>>& ref, std::size_t ref_n, std::complex<double> dc, std::complex<double> dz) -> std::tuple<std::size_t, std::complex<double>, std::complex<double>>;
template auto escape_perturbed<std::complex<double>>(const std::vector<std::complex<double>>& ref, std::complex<double> dc, std::size_t max_n, std::complex<double> dz, std::size_t n) -> std::pair<std::complex<double>, std::size_t>;
template auto compute_reference<std::complex<double>>(multi_complex c, std::size_t max_n, bool do_escape) -> std::vector<std::complex<double>>;

template auto rebase_reference<std::complex<long double>>(const std::vector<std::complex<long double>>& ref, std::size_t ref_n, std::complex<long double> dz) -> std::tuple<std::size_t, std::complex<long double>, std::complex<long double>>;
template auto compute_next_perturbation<std::complex<long double>>(const std::vector<std::complex<long double>>& ref, std::size_t ref_n, std::complex<long double> dc, std::complex<long double> dz) -> std::tuple<std::size_t, std::complex<long double>, std::complex<long double>>;
template auto escape_perturbed<std::complex<long double>>(const std::vector<std::complex<long double>>& ref, std::complex<long double> dc, std::size_t max_n, std::complex<long double> dz, std::size_t n) -> std::pair<std::complex<long double>, std::size_t>;
template auto compute_reference<std::complex<long double>>(multi_complex c, std::size_t max_n, bool do_escape) -> std::vector<std::complex<long double>>;

template auto rebase_reference<doubleexp_complex>(const std::vector<doubleexp_complex>& ref, std::size_t ref_n, doubleexp_complex dz) -> std::tuple<std::size_t, doubleexp_complex, doubleexp_complex>;
template auto compute_next_perturbation<doubleexp_complex>(const std::vector<doubleexp_complex>& ref, std::size_t ref_n, doubleexp_complex dc, doubleexp_complex dz) -> std::tuple<std::size_t, doubleexp_complex, doubleexp_complex>;
template auto escape_perturbed<doubleexp_complex>(const std::vector<doubleexp_complex>& ref, doubleexp_complex dc, std::size_t max_n, doubleexp_complex dz, std::size_t n) -> std::pair<doubleexp_complex, std::size_t>;
template auto compute_reference<doubleexp_complex>(multi_complex c, std::size_t max_n, bool do_escape) -> std::vector<doubleexp_complex>;
}   // namespace wacfrac
