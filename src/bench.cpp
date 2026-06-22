#include "wacfrac/constants.hpp"
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

constexpr unsigned long long get_fibonacci(size_t n) {
    if (n == 0) return 0;
    if (n == 1) return 1;
    
    unsigned long long a = 0;
    unsigned long long b = 1;
    
    for (size_t i = 2; i <= n; ++i) {
        unsigned long long next = a + b;
        a = b;
        b = next;
    }
    return b;
}

static void find_nucleus(benchmark::State& state) {
    auto c {wacfrac::poi::BIG_BANG};
    c.precision(state.range(0));
    for (auto _ : state) {
        auto nucleus {wacfrac::find_nucleus(c, state.range(1), state.range(2))};
        benchmark::DoNotOptimize(nucleus);
    }
}
BENCHMARK(find_nucleus)->ArgsProduct({
    {20, 100, 500, 1000}, // Precision
    { // Period
        get_fibonacci(2), 
        get_fibonacci(4),
        get_fibonacci(8),
        get_fibonacci(16),
        get_fibonacci(32),
        get_fibonacci(64),
    },
    {16, 32, 64, 128, 256, 512, 1028} // Max Iterations
})->Unit(benchmark::kMillisecond);
// static void find_period(benchmark::State& state);
// static void find_epsilon(benchmark::State& state);
// static void compute_sa_coeffs(benchmark::State& state);
// static void compute_bla_coeffs(benchmark::State& state);

// static void render_directly(benchmark::State& state);
// static void render_perturbed(benchmark::State& state);
// static void render_sa(benchmark::State& state);
// static void render_bla(benchmark::State& state);

BENCHMARK_MAIN();
