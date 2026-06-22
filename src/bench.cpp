#include "wacfrac/color.hpp"
#include <wacfrac/wacfrac.hpp>
#include <benchmark/benchmark.h>

using namespace boost::multiprecision;

template<wacfrac::Complex T>
static void compute_next_z(benchmark::State& state) {
    T z {0.0, 0.0};
    T c {0.0, 0.0};
    for (auto _ : state) {
        auto z_n = wacfrac::compute_next_z(z, c);
        benchmark::DoNotOptimize(z_n);
    }
}
BENCHMARK_TEMPLATE(compute_next_z, std::complex<float>);
BENCHMARK_TEMPLATE(compute_next_z, std::complex<double>);
BENCHMARK_TEMPLATE(compute_next_z, std::complex<long double>);
BENCHMARK_TEMPLATE(compute_next_z, wacfrac::doubleexp_complex);
BENCHMARK_TEMPLATE(compute_next_z, mpc_complex_100);
BENCHMARK_TEMPLATE(compute_next_z, mpc_complex_1000);

template<wacfrac::Complex T, std::size_t N = 256, std::size_t P = 256>
static void compute_reference(benchmark::State& state) {
    wacfrac::multi_complex c (0.0, 0.0, P);

    for (auto _ : state) {
        auto ref = wacfrac::compute_reference<T>(c, N);
        benchmark::DoNotOptimize(ref);
    }
}
BENCHMARK_TEMPLATE(compute_reference, std::complex<double>)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(compute_reference, std::complex<long double>)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(compute_reference, wacfrac::doubleexp_complex)->Unit(benchmark::kMicrosecond);

static void colorize_discrete(benchmark::State& state) {
    for (auto _ : state) {
        auto pix = wacfrac::colorize_discrete(
            wacfrac::colorization_method::looped,
            {{0,0,0},{255,255,255}},
            256, 0
        );
        benchmark::DoNotOptimize(pix);
    }
}
BENCHMARK(colorize_discrete);

static void colorize_continuous(benchmark::State& state) {
    for (auto _ : state) {
        auto pix = wacfrac::colorize_continuous(
            wacfrac::colorization_method::looped,
            {{0,0,0},{255,255,255}},
            256, {0.0,0.0}, 0
        );
        benchmark::DoNotOptimize(pix);
    }
}
BENCHMARK(colorize_continuous);

// static void find_nucleus(benchmark::State& state);
// static void find_period(benchmark::State& state);
// static void find_epsilon(benchmark::State& state);
// static void compute_sa_coeffs(benchmark::State& state);
// static void compute_bla_coeffs(benchmark::State& state);

// static void render_directly(benchmark::State& state);
// static void render_perturbed(benchmark::State& state);
// static void render_sa(benchmark::State& state);
// static void render_bla(benchmark::State& state);

BENCHMARK_MAIN();
