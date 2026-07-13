#include <cuda/std/span>
#include "wacfrac/bla.cuh"
#include "wacfrac/types.hpp"
#include "wacfrac/reference.hpp"
#include "wacfrac/viewport.hpp"
#include "wacfrac/log.hpp"
#include <cuda/buffer>
#include <cuda/devices>
#include <cuda/memory_pool>
#include <cuda/stream>
#include <cstdio>
#include <vector>
#include <cmath>

int main() {
    wacfrac::logging::init(0);

    std::printf("=== BLA GPU Calculator Test ===\n");

    wacfrac::MultiComplex::default_precision(100);
    wacfrac::MultiFloat::default_precision(100);
    wacfrac::MultiComplex c{"-0.75", "0.1", 100};
    constexpr unsigned max_n = 1000;

    std::printf("Computing reference orbit...\n");
    auto host_ref = wacfrac::compute_reference<wacfrac::DoubleExpComplex>(c, max_n, INFINITY);
    std::printf("Reference orbit: %zu points\n", host_ref.size());

    if (host_ref.size() < 10) {
        std::printf("ERROR: Reference orbit too short\n");
        return 1;
    }

    auto epsilon = static_cast<wacfrac::ComplexValueTypeT<wacfrac::DoubleExpComplex>>(1e-10);
    wacfrac::DoubleExpComplex max_dc{wacfrac::DoubleExp(1e-10), wacfrac::DoubleExp(1e-10)};
    constexpr std::size_t first_level = 0;

    std::printf("Running host BLA calculator...\n");
    wacfrac::bla::HostCalculator<wacfrac::DoubleExpComplex> host_calc(first_level);
    auto host_approx = host_calc.compute_manual(
        epsilon,
        std::span<const wacfrac::DoubleExpComplex>(host_ref),
        max_dc);
    auto host_cols = host_calc.get_columns();
    auto host_approx_span = host_calc.get_approximations();
    std::printf("Host: %zu columns, %zu approximations\n", host_cols.size(), host_approx_span.size());

    std::printf("Copying reference to GPU...\n");
    cuda::device_ref device{0};
    cuda::stream stream{device};
    auto device_ref_buf = cuda::make_buffer<wacfrac::DoubleExpComplex>(
        stream, cuda::device_default_memory_pool(device),
        host_ref.begin(), host_ref.end());
    cuda::std::span<const wacfrac::DoubleExpComplex> device_ref_span{device_ref_buf};

    std::printf("Running device BLA calculator...\n");
    wacfrac::bla::DeviceCalculator<wacfrac::DoubleExpComplex> device_calc(0, first_level);
    auto device_approx = device_calc.compute_manual(epsilon, device_ref_span, max_dc);

    auto dev_cols_span = device_calc.get_columns();
    std::vector<wacfrac::bla::ColumnInfo> dev_cols(dev_cols_span.size());
    cudaMemcpy(dev_cols.data(), dev_cols_span.data(),
               dev_cols_span.size() * sizeof(wacfrac::bla::ColumnInfo), cudaMemcpyDeviceToHost);

    auto dev_approx_span = device_calc.get_approximations();
    std::vector<wacfrac::bla::Bla<wacfrac::DoubleExpComplex>> dev_approxs(dev_approx_span.size());
    cudaMemcpy(dev_approxs.data(), dev_approx_span.data(),
               dev_approx_span.size() * sizeof(wacfrac::bla::Bla<wacfrac::DoubleExpComplex>),
               cudaMemcpyDeviceToHost);

    std::printf("Device: %zu columns, %zu approximations\n", dev_cols.size(), dev_approxs.size());

    int errors = 0;

    if (host_cols.size() != dev_cols.size()) {
        std::printf("FAIL: Column count mismatch: host=%zu device=%zu\n", host_cols.size(), dev_cols.size());
        ++errors;
    } else {
        for (std::size_t i = 0; i < host_cols.size(); ++i) {
            if (host_cols[i].first != dev_cols[i].first || host_cols[i].count != dev_cols[i].count) {
                std::printf("FAIL: Column[%zu] mismatch: host(%zu,%zu) device(%zu,%zu)\n",
                    i, host_cols[i].first, host_cols[i].count,
                    dev_cols[i].first, dev_cols[i].count);
                ++errors;
                if (errors > 10) { std::printf("... stopping after 10 errors\n"); break; }
            }
        }
    }

    if (host_approx_span.size() != dev_approxs.size()) {
        std::printf("FAIL: Approximation count mismatch: host=%zu device=%zu\n",
            host_approx_span.size(), dev_approxs.size());
        ++errors;
    } else {
        for (std::size_t i = 0; i < host_approx_span.size(); ++i) {
            auto& h = host_approx_span[i];
            auto& d = dev_approxs[i];
            bool a_match = (static_cast<double>(h.a.real()) == static_cast<double>(d.a.real()))
                        && (static_cast<double>(h.a.imag()) == static_cast<double>(d.a.imag()));
            bool b_match = (static_cast<double>(h.b.real()) == static_cast<double>(d.b.real()))
                        && (static_cast<double>(h.b.imag()) == static_cast<double>(d.b.imag()));
            bool r_match = (static_cast<double>(h.r) == static_cast<double>(d.r));
            if (!a_match || !b_match || !r_match) {
                std::printf("FAIL: Approximation[%zu] mismatch\n", i);
                ++errors;
                if (errors > 10) { std::printf("... stopping after 10 errors\n"); break; }
            }
        }
    }

    if (errors == 0) {
        std::printf("PASS: All %zu columns match\n", host_cols.size());
        std::printf("PASS: All %zu approximations match\n", host_approx_span.size());
    }

    std::printf("\n=== compute_search test ===\n");
    std::vector<wacfrac::DoubleExpComplex> probes;
    {
        wacfrac::MultiComplex::default_precision(100);
        wacfrac::MultiFloat::default_precision(100);
        wacfrac::MultiComplex dim{"4.0", "4.0", 100};
        wacfrac::Viewport view{c, dim};
        probes = view.generate_probes<wacfrac::DoubleExpComplex>(3, 3);
    }
    std::printf("Generated %zu probes\n", probes.size());

    std::printf("Running host compute_search...\n");
    wacfrac::bla::HostCalculator<wacfrac::DoubleExpComplex> host_search_calc(first_level);
    wacfrac::bla::SearchParams params{.lower_exp = -128.0, .upper_exp = 0.0, .tolerance = 1e-8};
    auto host_ok = host_search_calc.compute_search(params, probes, max_dc,
        std::span<const wacfrac::DoubleExpComplex>(host_ref));
    std::printf("Host compute_search returned: %s\n", host_ok ? "true" : "false");

    std::printf("Running device compute_search...\n");
    wacfrac::bla::DeviceCalculator<wacfrac::DoubleExpComplex> device_search_calc(0, first_level);
    auto device_ok = device_search_calc.compute_search(params, probes, max_dc, device_ref_span);
    std::printf("Device compute_search returned: %s\n", device_ok ? "true" : "false");

    if (!host_ok) {
        std::printf("FAIL: host compute_search returned false\n");
        ++errors;
    }
    if (!device_ok) {
        std::printf("FAIL: device compute_search returned false\n");
        ++errors;
    }

    auto host_search_cols = host_search_calc.get_columns();
    auto host_search_approxs = host_search_calc.get_approximations();
    auto device_search_cols_span = device_search_calc.get_columns();
    auto device_search_approxs_span = device_search_calc.get_approximations();
    std::vector<wacfrac::bla::ColumnInfo> dev_search_cols(device_search_cols_span.size());
    cudaMemcpy(dev_search_cols.data(), device_search_cols_span.data(),
               device_search_cols_span.size() * sizeof(wacfrac::bla::ColumnInfo), cudaMemcpyDeviceToHost);
    std::vector<wacfrac::bla::Bla<wacfrac::DoubleExpComplex>> dev_search_approxs(device_search_approxs_span.size());
    cudaMemcpy(dev_search_approxs.data(), device_search_approxs_span.data(),
               device_search_approxs_span.size() * sizeof(wacfrac::bla::Bla<wacfrac::DoubleExpComplex>),
               cudaMemcpyDeviceToHost);

    std::printf("Host search: %zu cols, %zu approxs\n", host_search_cols.size(), host_search_approxs.size());
    std::printf("Device search: %zu cols, %zu approxs\n", dev_search_cols.size(), dev_search_approxs.size());

    if (host_search_cols.size() != dev_search_cols.size()) {
        std::printf("FAIL: Search column count mismatch\n");
        ++errors;
    } else {
        for (std::size_t i = 0; i < host_search_cols.size(); ++i) {
            if (host_search_cols[i].first != dev_search_cols[i].first ||
                host_search_cols[i].count != dev_search_cols[i].count) {
                std::printf("FAIL: Search Column[%zu] mismatch\n", i);
                ++errors;
                if (errors > 20) break;
            }
        }
    }

    if (host_search_approxs.size() != dev_search_approxs.size()) {
        std::printf("FAIL: Search approximation count mismatch\n");
        ++errors;
    } else {
        for (std::size_t i = 0; i < host_search_approxs.size(); ++i) {
            auto& h = host_search_approxs[i];
            auto& d = dev_search_approxs[i];
            bool match = (static_cast<double>(h.a.real()) == static_cast<double>(d.a.real()))
                      && (static_cast<double>(h.a.imag()) == static_cast<double>(d.a.imag()))
                      && (static_cast<double>(h.b.real()) == static_cast<double>(d.b.real()))
                      && (static_cast<double>(h.b.imag()) == static_cast<double>(d.b.imag()))
                      && (static_cast<double>(h.r) == static_cast<double>(d.r));
            if (!match) {
                std::printf("FAIL: Search Approximation[%zu] mismatch\n", i);
                ++errors;
                if (errors > 20) break;
            }
        }
    }

    if (errors == 0) {
        std::printf("ALL TESTS PASSED\n");
    } else {
        std::printf("FAILED: %d errors\n", errors);
    }

    return errors == 0 ? 0 : 1;
}
