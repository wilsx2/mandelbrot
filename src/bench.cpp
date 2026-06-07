#include <wacfrac/wacfrac.hpp>
#include <benchmark/benchmark.h>

static void bm_render(benchmark::State& state) {
    wacfrac::multi_complex::default_precision(state.range(0));
    wacfrac::multi_float::default_precision(state.range(0));

    wacfrac::plot plot {
        {100uz, 100uz}, // Note: Magic
        wacfrac::FULL_SET,
        static_cast<std::size_t>(state.range(1))
    };
    std::vector<wacfrac::pixel> buffer (plot.res.width * plot.res.height);
    for (auto _ : state)
        plot.render(buffer);
}
BENCHMARK(bm_render)
    ->Args({5, 32});

BENCHMARK_MAIN();
