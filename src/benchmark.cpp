#include <wacfrac/wacfrac.h>
#include <benchmark/benchmark.h>

static void bm_render_full_mandelbrot(benchmark::State& state) {
    wacfrac::plot plot {
        .width  = wacfrac::resolution::hd_width;
        .height = wacfrac::resolution::hd_height;
        .limits = wacfrac::complete_view,
        .max_iterations = 10
        .precision
    }
}
BENCHMARK(bm_test);



BENCHMARK_MAIN();
