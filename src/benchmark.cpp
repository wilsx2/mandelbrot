#include "wacfrac/orbit.hpp"
#include "wacfrac/complex_adapter.hpp"
#include "wacfrac/floatexp2.hpp"
#include <wacfrac/wacfrac.hpp>
#include <benchmark/benchmark.h>
#include <filesystem>
#include <format>
using namespace boost::multiprecision;

namespace {

template <typename T>
constexpr auto type_short_name() -> std::string_view {
    if constexpr (std::is_same_v<T, wacfrac::Complex<float>>) return "single";
    if constexpr (std::is_same_v<T, wacfrac::Complex<double>>) return "double";
    if constexpr (std::is_same_v<T, wacfrac::DoubleExpComplex>) return "dexp";
    return "unknown";
}

auto make_viewport(unsigned zoom_exp) {
    auto zoom_factor {pow(wacfrac::MultiFloat(10.0), zoom_exp)};
    auto prec {wacfrac::required_precision(zoom_factor)};
    wacfrac::MultiFloat::default_precision(static_cast<unsigned>(prec));
    wacfrac::MultiComplex::default_precision(static_cast<unsigned>(prec));
    zoom_factor.precision(static_cast<unsigned>(prec));
    auto view {wacfrac::Viewport({0,0}, wacfrac::MultiComplex{1.0, 1.0}).zoomed(zoom_factor)};
    view.precision(static_cast<unsigned>(prec));
    return view;
}

auto compute_reference_dec(const wacfrac::Viewport& view, wacfrac::MultiComplex c_ref, unsigned max_iterations) {
    c_ref.precision(static_cast<unsigned>(view.required_precision()));
    return wacfrac::compute_reference<wacfrac::DoubleExpComplex>(c_ref, max_iterations);
}

void write_output(const std::string& name, std::span<const wacfrac::Pixel> pixels, const wacfrac::Resolution& res) {
    auto dir {std::filesystem::path("output")};
    std::filesystem::create_directories(dir);
    wacfrac::write_ppm((dir / (name + ".ppm")).string(), res, pixels);
}

auto count_mismatches(std::span<const wacfrac::Pixel> test, std::span<const wacfrac::Pixel> ref) -> std::size_t {
    std::size_t mismatches = 0;
    for (auto i : std::views::iota(0uz, test.size())) {
        auto [r1, g1, b1] = test[i];
        auto [r2, g2, b2] = ref[i];
        if (r1 != r2 || g1 != g2 || b1 != b2)
          mismatches++;
    }
    return mismatches;
}

} // anonymous namespace

// Operations

template<wacfrac::ComplexConcept T>
static void compute_next_orbit(benchmark::State& state) {
    T z {0.0, 0.0};
    unsigned n {0u};
    T c {0.0, 0.0};
    for (auto _ : state) {
        wacfrac::compute_next_orbit(z, n, c);
        benchmark::DoNotOptimize(z);
        benchmark::DoNotOptimize(n);
        benchmark::DoNotOptimize(c);
    }
}
BENCHMARK_TEMPLATE(compute_next_orbit, wacfrac::Complex<float>);
BENCHMARK_TEMPLATE(compute_next_orbit, wacfrac::Complex<double>);
BENCHMARK_TEMPLATE(compute_next_orbit, wacfrac::DoubleExpComplex);
#ifdef BOOST_MP_HAVE_MPC
BENCHMARK_TEMPLATE(compute_next_orbit, boost::multiprecision::mpc_complex_100);
BENCHMARK_TEMPLATE(compute_next_orbit, boost::multiprecision::mpc_complex_1000);
#endif

template<wacfrac::ComplexConcept T>
static void compute_next_dz(benchmark::State& state) {
    T z  {0.0, 0.0};
    T dz {0.0, 0.0};
    T dc {0.0, 0.0};
    for (auto _ : state) {
        auto dz_n {T{2.0, 0.0} * dz * z + dz * dz + dc};
        benchmark::DoNotOptimize(dz_n);
    }
}
BENCHMARK_TEMPLATE(compute_next_dz, wacfrac::Complex<float>);
BENCHMARK_TEMPLATE(compute_next_dz, wacfrac::Complex<double>);
BENCHMARK_TEMPLATE(compute_next_dz, wacfrac::DoubleExpComplex);

template<wacfrac::ComplexConcept T>
static void compute_reference_bench(benchmark::State& state) {
    auto scale {boost::multiprecision::pow(wacfrac::MultiFloat(10.0),-state.range(0))};
    wacfrac::MultiComplex c {0.0,0.0};
    c.precision(static_cast<unsigned>(wacfrac::required_precision(scale)));
    for (auto _ : state) {
        auto ref {wacfrac::compute_reference<T>(c, wacfrac::required_iterations(scale))};
        benchmark::DoNotOptimize(ref);
    }
}
BENCHMARK_TEMPLATE(compute_reference_bench, wacfrac::Complex<float>)->RangeMultiplier(2)->Range(0, 1024)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(compute_reference_bench, wacfrac::Complex<double>)->RangeMultiplier(2)->Range(0, 1024)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(compute_reference_bench, wacfrac::DoubleExpComplex)->RangeMultiplier(2)->Range(0, 1024)->Unit(benchmark::kMillisecond);

template<wacfrac::ComplexConcept T>
static void compute_reference_mt_bench(benchmark::State& state) {
    auto scale {boost::multiprecision::pow(wacfrac::MultiFloat(10.0),-state.range(0))};
    wacfrac::MultiComplex c {0.0,0.0};
    c.precision(static_cast<unsigned>(wacfrac::required_precision(scale)));
    for (auto _ : state) {
        auto ref {wacfrac::compute_reference_mt<T>(c, wacfrac::required_iterations(scale))};
        benchmark::DoNotOptimize(ref);
    }
}
BENCHMARK_TEMPLATE(compute_reference_mt_bench, wacfrac::Complex<float>)->RangeMultiplier(2)->Range(0, 1024)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(compute_reference_mt_bench, wacfrac::Complex<double>)->RangeMultiplier(2)->Range(0, 1024)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(compute_reference_mt_bench, wacfrac::DoubleExpComplex)->RangeMultiplier(2)->Range(0, 1024)->Unit(benchmark::kMillisecond);

static void find_period(benchmark::State& state) {
    auto scale {boost::multiprecision::pow(wacfrac::MultiFloat(10.0),-state.range(0))};
    wacfrac::MultiComplex c {0.0,0.0};
    c.precision(static_cast<unsigned>(wacfrac::required_precision(scale)));
    for (auto _ : state) {
        auto period {wacfrac::PeriodFinder(c, scale / 2.0, scale / 2.0, wacfrac::required_iterations(scale)).next()};
        benchmark::DoNotOptimize(period);
    }
}
BENCHMARK(find_period)->RangeMultiplier(2)->Range(0, 1024)->Unit(benchmark::kMillisecond);

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
    auto view {make_viewport(static_cast<unsigned>(state.range(0)))};
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

template<typename T>
static void compute_bla_coefficients(benchmark::State& state) {
    auto view   {make_viewport(static_cast<unsigned>(state.range(0)))};
    auto c_ref  {view.center};
    auto ref    {wacfrac::compute_reference<T>(c_ref, view.required_iterations())};
    auto max_dc {wacfrac::to_complex<T>(view.compute_max_dc(c_ref))};
    auto probes {view.generate_probes<T>(state.range(1), state.range(1))};
    for (auto _ : state) {
        wacfrac::BivariateLinearApproximator<T> bla {
            -256.0, 0.0, std::pow(10.0, -state.range(2)), probes, max_dc, ref, 0
        };
        benchmark::DoNotOptimize(bla);
    }
}
BENCHMARK_TEMPLATE(compute_bla_coefficients, wacfrac::Complex<float>)->ArgsProduct({
    {0, 25, 125, 250, 500, 1000, 2000}, // Zoom Factor Exponent
    {1, 2, 4, 8}, // Probe Rows/Cols
    {1, 2, 4, 8}, // Tolerance Negative Exponent
})->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(compute_bla_coefficients, wacfrac::Complex<double>)->ArgsProduct({
    {0, 25, 125, 250, 500, 1000, 2000}, // Zoom Factor Exponent
    {1, 2, 4, 8}, // Probe Rows/Cols
    {1, 2, 4, 8}, // Tolerance Negative Exponent
})->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(compute_bla_coefficients, wacfrac::DoubleExpComplex)->ArgsProduct({
    {0, 25, 125, 250, 500, 1000, 2000}, // Zoom Factor Exponent
    {1, 2, 4, 8}, // Probe Rows/Cols
    {1, 2, 4, 8}, // Tolerance Negative Exponent
})->Unit(benchmark::kMillisecond);

// Per pixel render

template <wacfrac::ComplexConcept T>
static void render_phase_direct(benchmark::State& state) {
    auto zoom_exp {static_cast<unsigned>(state.range(0))};
    auto dim {static_cast<std::size_t>(state.range(1))};
    wacfrac::Resolution res {dim, dim};
    auto view {make_viewport(zoom_exp)};
    auto max_iterations {view.required_iterations()};

    std::vector<wacfrac::Pixel> pixels(res.area());
    for (auto _ : state) {
        auto cs {wacfrac::sample_c_values<T>(view, res)};
        auto escaped_orbits {cs | std::views::transform([&](auto c){
            return wacfrac::escape(c, max_iterations, 2.0);
        })};
        for (auto&& [pixel, orbit] : std::views::zip(pixels, escaped_orbits)) {
            pixel = wacfrac::colorize_discrete(std::get<1>(orbit), max_iterations, wacfrac::ULTRA);
        }
    }
    state.SetComplexityN(res.area());
    state.PauseTiming();
    write_output(std::format("phase_direct_{}_{}x{}_z{}", type_short_name<T>(), dim, dim, zoom_exp), pixels, res);
    state.ResumeTiming();
}
BENCHMARK_TEMPLATE(render_phase_direct, wacfrac::Complex<float>)->ArgsProduct({{0, 10, 25}, {32, 64, 128}})->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(render_phase_direct, wacfrac::Complex<double>)->ArgsProduct({{0, 10, 25}, {32, 64, 128}})->Unit(benchmark::kMillisecond);

template <wacfrac::ComplexConcept T>
static void render_phase_perturbed(benchmark::State& state) {
    auto zoom_exp {static_cast<unsigned>(state.range(0))};
    auto dim {static_cast<std::size_t>(state.range(1))};
    wacfrac::Resolution res{dim, dim};
    auto view {make_viewport(zoom_exp)};
    auto max_iterations {view.required_iterations()};
    auto c_ref {view.center};

    auto ref {compute_reference_dec(view, c_ref, max_iterations)};

    std::vector<wacfrac::Pixel> pixels(res.area());
    for (auto _ : state) {
        auto dcs {wacfrac::sample_c_values<T>(view, res, wacfrac::to_complex<T>(c_ref))};
        auto escaped_orbits {dcs | std::views::transform([&](auto dc){
            return wacfrac::escape_perturbed(dc, std::span<const T>(ref), max_iterations, 2.0);
        })};
        for (auto&& [pixel, orbit] : std::views::zip(pixels, escaped_orbits)) {
            pixel = wacfrac::colorize_discrete(std::get<1>(orbit), max_iterations, wacfrac::ULTRA);
        }
    }
    state.SetComplexityN(res.area());
    state.PauseTiming();
    write_output(std::format("phase_perturbed_{}_{}x{}_z{}", type_short_name<T>(), dim, dim, zoom_exp), pixels, res);
    state.ResumeTiming();
}
BENCHMARK_TEMPLATE(render_phase_perturbed, wacfrac::DoubleExpComplex)->ArgsProduct({{0, 25, 125}, {64, 128}})->Unit(benchmark::kMillisecond);

template <wacfrac::ComplexConcept T>
static void render_phase_bla(benchmark::State& state) {
    auto zoom_exp {static_cast<unsigned>(state.range(0))};
    auto dim {static_cast<std::size_t>(state.range(1))};
    wacfrac::Resolution res{dim, dim};
    auto view {make_viewport(zoom_exp)};
    auto max_iterations {view.required_iterations()};
    auto c_ref {view.center};

    auto ref {compute_reference_dec(view, c_ref, max_iterations)};
    auto last_level {static_cast<std::size_t>(std::log2(ref.size()))};
    auto first_level {std::max(0uz, last_level > 9 ? last_level - 9 : 0uz)};
    auto max_dc {wacfrac::to_complex<T>(view.compute_max_dc(c_ref))};
    auto probes {view.generate_probes<T>(3, 3)};
    wacfrac::BivariateLinearApproximator<T> bla{-256.0, 0.0, 1e-8, probes, max_dc, ref, first_level};

    std::vector<wacfrac::Pixel> pixels(res.area());
    for (auto _ : state) {
        auto dcs {wacfrac::sample_c_values<T>(view, res, wacfrac::to_complex<T>(c_ref))};
        auto escaped_orbits {dcs | std::views::transform([&](auto dc){
            return bla.escape_approximate(dc);
        })};
        for (auto&& [pixel, orbit] : std::views::zip(pixels, escaped_orbits)) {
            pixel = wacfrac::colorize_discrete(std::get<1>(orbit), max_iterations, wacfrac::ULTRA);
        }
    }
    state.SetComplexityN(res.area());
    state.PauseTiming();
    write_output(std::format("phase_bla_{}_{}x{}_z{}", type_short_name<T>(), dim, dim, zoom_exp), pixels, res);
    state.ResumeTiming();
}
BENCHMARK_TEMPLATE(render_phase_bla, wacfrac::DoubleExpComplex)->ArgsProduct({{0, 25, 125}, {64, 128}})->Unit(benchmark::kMillisecond);

// E2E Pipeline

template <wacfrac::ComplexConcept T>
static void e2e_direct(benchmark::State& state) {
    auto zoom_exp {static_cast<unsigned>(state.range(0))};
    auto dim {static_cast<std::size_t>(state.range(1))};
    wacfrac::Resolution res{dim, dim};

    std::vector<wacfrac::Pixel> pixels(res.area());
    for (auto _ : state) {
        auto view {make_viewport(zoom_exp)};
        auto max_iterations {view.required_iterations()};
        auto cs {wacfrac::sample_c_values<T>(view, res)};
        auto escaped_orbits {cs | std::views::transform([&](auto c){
            return wacfrac::escape(c, max_iterations, 2.0);
        })};
        for (auto&& [pixel, orbit] : std::views::zip(pixels, escaped_orbits)) {
            pixel = wacfrac::colorize_discrete(std::get<1>(orbit), max_iterations, wacfrac::ULTRA);
        }
    }
    state.PauseTiming();
    auto view {make_viewport(zoom_exp)};
    auto max_iterations {view.required_iterations()};
    std::vector<wacfrac::Pixel> ref_pixels(res.area());
    auto ref_cs {wacfrac::sample_c_values<T>(view, res)};
    auto ref_escaped_orbits {ref_cs | std::views::transform([&](auto c){
        return wacfrac::escape(c, max_iterations, 2.0);
    })};
    for (auto&& [pixel, orbit] : std::views::zip(ref_pixels, ref_escaped_orbits)) {
        pixel = wacfrac::colorize_discrete(std::get<1>(orbit), max_iterations, wacfrac::ULTRA);
    }
    state.counters["mismatch"] = static_cast<double>(count_mismatches(pixels, ref_pixels));
    write_output(std::format("e2e_direct_{}_{}x{}_z{}", type_short_name<T>(), dim, dim, zoom_exp), pixels, res);
    state.ResumeTiming();
}
BENCHMARK_TEMPLATE(e2e_direct, wacfrac::Complex<double>)->ArgsProduct({{0}, {32, 64}})->Unit(benchmark::kMillisecond);

template <wacfrac::ComplexConcept T>
static void e2e_perturbed(benchmark::State& state) {
    auto zoom_exp {static_cast<unsigned>(state.range(0))};
    auto dim {static_cast<std::size_t>(state.range(1))};
    wacfrac::Resolution res{dim, dim};

    std::vector<wacfrac::Pixel> pixels(res.area());
    for (auto _ : state) {
        auto view {make_viewport(zoom_exp)};
        auto max_iterations {view.required_iterations()};
        auto c_ref {view.center};
        auto ref {compute_reference_dec(view, c_ref, max_iterations)};
        auto dcs {wacfrac::sample_c_values<T>(view, res, wacfrac::to_complex<T>(c_ref))};
        auto escaped_orbits {dcs | std::views::transform([&](auto dc){
            return wacfrac::escape_perturbed(dc, std::span<const T>(ref), max_iterations, 2.0);
        })};
        for (auto&& [pixel, orbit] : std::views::zip(pixels, escaped_orbits)) {
            pixel = wacfrac::colorize_discrete(std::get<1>(orbit), max_iterations, wacfrac::ULTRA);
        }
    }
    state.PauseTiming();
    auto view {make_viewport(zoom_exp)};
    auto max_iterations {view.required_iterations()};
    auto c_ref {view.center};
    auto ref {compute_reference_dec(view, c_ref, max_iterations)};
    std::vector<wacfrac::Pixel> ref_pixels(res.area());
    auto ref_dcs {wacfrac::sample_c_values<T>(view, res, wacfrac::to_complex<T>(c_ref))};
    auto ref_escaped_orbits {ref_dcs | std::views::transform([&](auto dc){
        return wacfrac::escape_perturbed(dc, std::span<const T>(ref), max_iterations, 2.0);
    })};
    for (auto&& [pixel, orbit] : std::views::zip(ref_pixels, ref_escaped_orbits)) {
        pixel = wacfrac::colorize_discrete(std::get<1>(orbit), max_iterations, wacfrac::ULTRA);
    }
    state.counters["mismatch"] = static_cast<double>(count_mismatches(pixels, ref_pixels));
    write_output(std::format("e2e_perturbed_{}_{}x{}_z{}", type_short_name<T>(), dim, dim, zoom_exp), pixels, res);
    state.ResumeTiming();
}
BENCHMARK_TEMPLATE(e2e_perturbed, wacfrac::DoubleExpComplex)->ArgsProduct({{0, 25, 125}, {64}})->Unit(benchmark::kMillisecond);

template <wacfrac::ComplexConcept T>
static void e2e_bla(benchmark::State& state) {
    auto zoom_exp {static_cast<unsigned>(state.range(0))};
    auto dim {static_cast<std::size_t>(state.range(1))};
    wacfrac::Resolution res{dim, dim};

    std::vector<wacfrac::Pixel> pixels(res.area());
    for (auto _ : state) {
        auto view {make_viewport(zoom_exp)};
        auto max_iterations {view.required_iterations()};
        auto c_ref {view.center};
        auto ref {compute_reference_dec(view, c_ref, max_iterations)};
        auto last_level {static_cast<std::size_t>(std::log2(ref.size()))};
        auto first_level {std::max(0uz, last_level > 9 ? last_level - 9 : 0uz)};
        auto max_dc {wacfrac::to_complex<T>(view.compute_max_dc(c_ref))};
        auto probes {view.generate_probes<T>(3, 3)};
        wacfrac::BivariateLinearApproximator<T> bla {-256.0, 0.0, 1e-8, probes, max_dc, ref, first_level};
        auto dcs {wacfrac::sample_c_values<T>(view, res, wacfrac::to_complex<T>(c_ref))};
        auto escaped_orbits {dcs | std::views::transform([&](auto dc){
            return bla.escape_approximate(dc);
        })};
        for (auto&& [pixel, orbit] : std::views::zip(pixels, escaped_orbits)) {
            pixel = wacfrac::colorize_discrete(std::get<1>(orbit), max_iterations, wacfrac::ULTRA);
        }
    }
    state.PauseTiming();
    auto view {make_viewport(zoom_exp)};
    auto max_iterations {view.required_iterations()};
    auto c_ref {view.center};
    auto ref {compute_reference_dec(view, c_ref, max_iterations)};
    std::vector<wacfrac::Pixel> ref_pixels(res.area());
    auto ref_dcs {wacfrac::sample_c_values<T>(view, res, wacfrac::to_complex<T>(c_ref))};
    auto ref_escaped_orbits {ref_dcs | std::views::transform([&](auto dc){
        return wacfrac::escape_perturbed(dc, std::span<const T>(ref), max_iterations, 2.0);
    })};
    for (auto&& [pixel, orbit] : std::views::zip(ref_pixels, ref_escaped_orbits)) {
        pixel = wacfrac::colorize_discrete(std::get<1>(orbit), max_iterations, wacfrac::ULTRA);
    }
    state.counters["mismatch"] = static_cast<double>(count_mismatches(pixels, ref_pixels));
    write_output(std::format("e2e_bla_{}_{}x{}_z{}", type_short_name<T>(), dim, dim, zoom_exp), pixels, res);
    state.ResumeTiming();
}
BENCHMARK_TEMPLATE(e2e_bla, wacfrac::DoubleExpComplex)->ArgsProduct({{0, 25, 125}, {64}})->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
