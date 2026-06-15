#include <wacfrac/wacfrac.hpp>
#include <benchmark/benchmark.h>

using namespace boost::multiprecision;

template<typename T>
    // TODO: Concept
static void calculate_next_z(benchmark::State& state) {
    wacfrac::orbit<T> o {{0.0, 0.0}};
    for (auto _ : state) {
        o.iterate();
    }
}
BENCHMARK_TEMPLATE(calculate_next_z, std::complex<float>);
BENCHMARK_TEMPLATE(calculate_next_z, std::complex<double>);
BENCHMARK_TEMPLATE(calculate_next_z, mpc_complex_50);
BENCHMARK_TEMPLATE(calculate_next_z, mpc_complex_100);
BENCHMARK_TEMPLATE(calculate_next_z, mpc_complex_500);
BENCHMARK_TEMPLATE(calculate_next_z, mpc_complex_1000);

BENCHMARK_MAIN();
