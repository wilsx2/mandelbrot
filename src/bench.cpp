#include <wacfrac/wacfrac.hpp>
#include <benchmark/benchmark.h>

using namespace boost::multiprecision;

template<wacfrac::Complex T>
static void calculate_next_z(benchmark::State& state) {
    T z {0.0, 0.0};
    T c {0.0, 0.0};
    for (auto _ : state) {
        auto z_n = wacfrac::compute_next_z(z, c);
        benchmark::DoNotOptimize(z_n);
    }
}
BENCHMARK_TEMPLATE(calculate_next_z, std::complex<float>);
BENCHMARK_TEMPLATE(calculate_next_z, std::complex<double>);
BENCHMARK_TEMPLATE(calculate_next_z, mpc_complex_50);
BENCHMARK_TEMPLATE(calculate_next_z, mpc_complex_100);
BENCHMARK_TEMPLATE(calculate_next_z, mpc_complex_500);
BENCHMARK_TEMPLATE(calculate_next_z, mpc_complex_1000);

BENCHMARK_MAIN();
