#include "wacfrac/constants.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/types.hpp"
#include "wacfrac/viewport.hpp"
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
BENCHMARK_TEMPLATE(compute_next_z, wacfrac::DoubleExpComplex);
BENCHMARK_TEMPLATE(compute_next_z, mpc_complex_100);
BENCHMARK_TEMPLATE(compute_next_z, mpc_complex_1000);

template<wacfrac::Complex T>
static void compute_next_dz(benchmark::State& state) {
    T z {0.0, 0.0};
    T dz {0.0, 0.0};
    T dc {0.0, 0.0};
    for (auto _ : state) {
        auto dz_n = T{2.0, 0.0} * dz * z + dz * dz + dc;
        benchmark::DoNotOptimize(dz_n);
    }
}
BENCHMARK_TEMPLATE(compute_next_dz, std::complex<float>);
BENCHMARK_TEMPLATE(compute_next_dz, std::complex<double>);
BENCHMARK_TEMPLATE(compute_next_dz, std::complex<long double>);
BENCHMARK_TEMPLATE(compute_next_dz, wacfrac::DoubleExpComplex);

template<wacfrac::Complex T>
static void compute_reference(benchmark::State& state) {
    auto scale {boost::multiprecision::pow(wacfrac::MultiFloat(10.0),-state.range(0))};
    auto c {wacfrac::poi::BIG_BANG};
    c.precision(wacfrac::required_precision(scale));
    for (auto _ : state) {
        auto ref {wacfrac::compute_reference<T>(c, wacfrac::required_iterations(scale))};
        benchmark::DoNotOptimize(ref);
    }
}
BENCHMARK_TEMPLATE(compute_reference, std::complex<double>)->RangeMultiplier(2)->Range(0, 1024)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(compute_reference, std::complex<long double>)->RangeMultiplier(2)->Range(0, 1024)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(compute_reference, wacfrac::DoubleExpComplex)->RangeMultiplier(2)->Range(0, 1024)->Unit(benchmark::kMillisecond);

static void colorize_looped_bench(benchmark::State& state) {
    for (auto _ : state) {
        auto pix {wacfrac::colorize_looped(
            {{0,0,0},{255,255,255}},
            0
        )};
        benchmark::DoNotOptimize(pix);
    }
}
BENCHMARK(colorize_looped_bench);

static void colorize_continuous_bench(benchmark::State& state) {
    auto lookup = [](std::size_t n) -> wacfrac::Pixel {
        return wacfrac::colorize_looped({{0,0,0},{255,255,255}}, n);
    };
    for (auto _ : state) {
        auto pix = wacfrac::colorize_continuous(
            lookup, {0.0f, 0.0f}, 0
        );
        benchmark::DoNotOptimize(pix);
    }
}
BENCHMARK(colorize_continuous_bench);

static void find_periods(benchmark::State& state) {
    auto scale {boost::multiprecision::pow(wacfrac::MultiFloat(10.0),-state.range(0))};
    auto c {wacfrac::poi::BIG_BANG};
    c.precision(wacfrac::required_precision(scale));
    for (auto _ : state) {
        auto periods {wacfrac::find_period_ball(c, scale / 2.0, scale / 2.0, wacfrac::required_iterations(scale), true)};
        benchmark::DoNotOptimize(periods);
    }
}
BENCHMARK(find_periods)->RangeMultiplier(2)->Range(0, 1024)->Unit(benchmark::kMillisecond);

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
    auto zoom_factor {boost::multiprecision::pow(wacfrac::MultiFloat(10.0),state.range(0))};
    auto view = wacfrac::Viewport(wacfrac::poi::BIG_BANG, wacfrac::MultiComplex{1.0} / zoom_factor);
    wacfrac::MultiComplex::default_precision(view.required_precision());
    for (auto _ : state) {
        auto nucleus {wacfrac::find_nucleus(view.center, state.range(1), 256)};
        benchmark::DoNotOptimize(nucleus);
    }
}
BENCHMARK(find_nucleus)->ArgsProduct({
    {0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024}, // Zoom Exponent
    { // Period
        get_fibonacci(2), 
        get_fibonacci(4),
        get_fibonacci(8),
        get_fibonacci(16)
    },
})->Unit(benchmark::kMillisecond);

template <wacfrac::Complex T>
static void compute_sa_coefficients(benchmark::State& state) {
    auto zoom_factor {boost::multiprecision::pow(wacfrac::MultiFloat(10.0),state.range(0))};
    auto view = wacfrac::Viewport(wacfrac::poi::BIG_BANG, 1.0).zoomed(zoom_factor);
    wacfrac::MultiComplex::default_precision(view.required_precision());

    auto reference = wacfrac::compute_reference<T>(view.center, view.required_iterations());
    auto probes = view.generate_probes<T>(state.range(2), state.range(2));
    for (auto _ : state) {
        wacfrac::SeriesApproximator<T> sa {
            reference,
            static_cast<std::size_t>(state.range(1)),
            probes,
            std::pow(10.0, -state.range(3))
        };
        benchmark::DoNotOptimize(sa);
    }
}
BENCHMARK_TEMPLATE(compute_sa_coefficients, std::complex<double>)->ArgsProduct({
    {0, 25, 125, 250, 500, 1000, 2000}, // Zoom Factor Exponent
    {1, 2, 4, 8, 16, 32, 64}, // Num Coeffs
    {1, 2, 4, 8}, // Probe Rows/Cols
    {1, 2, 4, 8}, // Tolerance Negative Exponent
})->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(compute_sa_coefficients, std::complex<long double>)->ArgsProduct({
    {0, 25, 125, 250, 500, 1000, 2000}, // Zoom Factor Exponent
    {1, 2, 4, 8, 16, 32, 64}, // Num Coeffs
    {1, 2, 4, 8}, // Probe Rows/Cols
    {1, 2, 4, 8}, // Tolerance Negative Exponent
})->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(compute_sa_coefficients, wacfrac::DoubleExpComplex)->ArgsProduct({
    {0, 25, 125, 250, 500, 1000, 2000}, // Zoom Factor Exponent
    {1, 2, 4, 8, 16, 32, 64}, // Num Coeffs
    {1, 2, 4, 8}, // Probe Rows/Cols
    {1, 2, 4, 8}, // Tolerance Negative Exponent
})->Unit(benchmark::kMillisecond);

template<typename T>
static void compute_bla_coefficients(benchmark::State& state) {
    auto zoom_factor {boost::multiprecision::pow(wacfrac::MultiFloat(10.0),state.range(0))};
    auto view = wacfrac::Viewport(wacfrac::poi::BIG_BANG, 1.0).zoomed(zoom_factor);
    wacfrac::MultiComplex::default_precision(view.required_precision());

    wacfrac::MultiComplex c_ref = view.center;
    auto ref = wacfrac::compute_reference<T>(c_ref, view.required_iterations(), true);
    auto max_dc = wacfrac::to_complex<T>(view.compute_max_dc(c_ref));
    auto probes = view.generate_probes<T>(state.range(1), state.range(1));
    for (auto _ : state) {
        wacfrac::BivariateLinearApproximator<T> bla {
            std::pow(10.0, -state.range(3)), probes, max_dc, ref, 0
        };
        benchmark::DoNotOptimize(bla);
    }
}
BENCHMARK_TEMPLATE(compute_bla_coefficients, std::complex<double>)->ArgsProduct({
    {0, 25, 125, 250, 500, 1000, 2000}, // Zoom Factor Exponent
    {1, 2, 4, 8}, // Probe Rows/Cols
    {1, 2, 4, 8}, // Tolerance Negative Exponent
})->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(compute_bla_coefficients, std::complex<long double>)->ArgsProduct({
    {0, 25, 125, 250, 500, 1000, 2000}, // Zoom Factor Exponent
    {1, 2, 4, 8}, // Probe Rows/Cols
    {1, 2, 4, 8}, // Tolerance Negative Exponent
})->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(compute_bla_coefficients, wacfrac::DoubleExpComplex)->ArgsProduct({
    {0, 25, 125, 250, 500, 1000, 2000}, // Zoom Factor Exponent
    {1, 2, 4, 8}, // Probe Rows/Cols
    {1, 2, 4, 8}, // Tolerance Negative Exponent
})->Unit(benchmark::kMillisecond);

// static void e2e_direct(benchmark::State& state);
// static void e2e_perturbed(benchmark::State& state);
// static void e2e_sa(benchmark::State& state);
// static void e2e_bla(benchmark::State& state);

BENCHMARK_MAIN();
