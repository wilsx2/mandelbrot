#include <wacfrac/wacfrac.hpp>
#include <benchmark/benchmark.h>

static void bm_render_standard(benchmark::State& state) {
    wacfrac::multi_complex::default_precision(10); // Note: Magic
    wacfrac::multi_float::default_precision(10); // Note: Magic

    wacfrac::plot plot {
        wacfrac::video_resolution::SD360p,
        wacfrac::FULL_SET,
        64 // Note: Magic
    };
    std::vector<wacfrac::pixel> buffer (plot.res.width * plot.res.height);
    for (auto _ : state)
        plot.render(buffer);
}
BENCHMARK(bm_render_standard);
BENCHMARK_MAIN();
