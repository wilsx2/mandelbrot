#include "wacfrac/bla.hpp"
#include <cuda_runtime.h>
#include <iostream>
#include <cstring>
#include <tuple>

using namespace wacfrac;

#define CUDA_CHECK(call) do { \
    cudaError_t err = (call); \
    if (err != cudaSuccess) { \
        std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ \
                  << " - " << cudaGetErrorString(err) << std::endl; \
        return false; \
    } \
} while(0)

namespace {

using T = DoubleComplex;
using CT = ComplexValueTypeT<T>;
constexpr double APPROX_TOL = 1e-10;
constexpr float Z_TOL = 0.01f;

auto make_reference(unsigned n) -> std::vector<T> {
    std::vector<T> ref;
    ref.reserve(n);
    T z{0.0, 0.0};
    T c{-0.75, 0.1};
    ref.push_back(z);
    for (unsigned i = 1; i < n; ++i) {
        z = z * z + c;
        ref.push_back(z);
    }
    return ref;
}

auto make_probes() -> std::vector<T> {
    return {
        {0.001, 0.001}, {-0.001, 0.001}, {0.001, -0.001}, {-0.001, -0.001},
        {0.0005, 0.0}, {-0.0005, 0.0}, {0.0, 0.0005}, {0.0, -0.0005},
        {0.0, 0.0},
    };
}

auto bla_equal(const bla::Bla<T>& a, const bla::Bla<T>& b, double tol = APPROX_TOL) -> bool {
    using std::abs;
    auto approx_eq = [](auto x, auto y, double t) {
        auto d = abs(x - y);
        auto s = abs(x) + abs(y);
        return d <= t || d <= s * t || (d != d);
    };
    return approx_eq(a.a, b.a, tol) && approx_eq(a.b, b.b, tol) && approx_eq(a.r, b.r, tol);
}

auto columns_equal(const bla::ColumnInfo& a, const bla::ColumnInfo& b) -> bool {
    return a.start == b.start && a.count == b.count;
}

template<typename U>
__global__ void escape_kernel(
    const U* dcs, unsigned num_dcs,
    const U* ref, unsigned ref_size,
    double escape_radius,
    bla::Approximator<U> approximator,
    Complex<float>* z_out,
    unsigned* n_out,
    unsigned* skipped_out)
{
    unsigned tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= num_dcs) return;

    auto [z, n, skipped] = bla::escape_approximate(
        dcs[tid], WF_STD::span<const U>(ref, ref_size), ref_size, escape_radius, approximator);
    z_out[tid] = z;
    n_out[tid] = n;
    skipped_out[tid] = skipped;
}

auto test_compute_manual(unsigned ref_size, std::size_t first_level) -> bool {
    auto ref = make_reference(ref_size);
    auto max_dc = T{0.001, 0.001};
    CT epsilon = 1e-10;

    bla::Calculator<Host, T> host_calc{{.first_level = 0}};
    host_calc.compute_manual(epsilon, WF_STD::span<const T>(ref.data(), ref.size()), max_dc);
    auto host_approx = host_calc.get_approximator();

    T* d_ref = nullptr;
    CUDA_CHECK(cudaMallocManaged(&d_ref, ref_size * sizeof(T)));
    std::memcpy(d_ref, ref.data(), ref_size * sizeof(T));

    bla::Calculator<Device, T> device_calc{{.first_level = 0}};
    device_calc.compute_manual(epsilon, WF_STD::span<const T>(d_ref, ref_size), max_dc);
    auto device_approx = device_calc.get_approximator();

    bool ok = true;

    if (host_approx.columns.size() != device_approx.columns.size()) {
        std::cerr << "  Column count mismatch: " << host_approx.columns.size()
                  << " vs " << device_approx.columns.size() << std::endl;
        ok = false;
    } else {
        for (std::size_t i = 0; i < host_approx.columns.size(); ++i) {
            if (!columns_equal(host_approx.columns[i], device_approx.columns[i])) {
                std::cerr << "  Column[" << i << "]: host=(" << host_approx.columns[i].start
                          << "," << host_approx.columns[i].count << ") device=("
                          << device_approx.columns[i].start << "," << device_approx.columns[i].count
                          << ")" << std::endl;
                ok = false;
            }
        }
    }

    if (host_approx.approximations.size() != device_approx.approximations.size()) {
        std::cerr << "  Approximation count mismatch: " << host_approx.approximations.size()
                  << " vs " << device_approx.approximations.size() << std::endl;
        ok = false;
    } else {
        for (std::size_t i = 0; i < host_approx.approximations.size(); ++i) {
            if (!bla_equal(host_approx.approximations[i], device_approx.approximations[i])) {
                auto& a = host_approx.approximations[i];
                auto& b = device_approx.approximations[i];
                std::cerr << "  Approx[" << i
                          << "]: host a=(" << a.a.real() << "," << a.a.imag()
                          << ") b=(" << a.b.real() << "," << a.b.imag() << ") r=" << a.r
                          << " device a=(" << b.a.real() << "," << b.a.imag()
                          << ") b=(" << b.b.real() << "," << b.b.imag() << ") r=" << b.r
                          << std::endl;
                ok = false;
            }
        }
    }

    CUDA_CHECK(cudaFree(d_ref));
    return ok;
}

auto test_escape_approximate() -> bool {
    constexpr unsigned REF_SIZE = 100;
    auto ref = make_reference(REF_SIZE);
    auto max_dc = T{0.001, 0.001};
    CT epsilon = 1e-10;

    bla::Calculator<Host, T> host_calc{{.first_level = 0}};
    host_calc.compute_manual(epsilon, WF_STD::span<const T>(ref.data(), ref.size()), max_dc);
    auto host_approx = host_calc.get_approximator();

    T* d_ref = nullptr;
    CUDA_CHECK(cudaMallocManaged(&d_ref, REF_SIZE * sizeof(T)));
    std::memcpy(d_ref, ref.data(), REF_SIZE * sizeof(T));

    bla::Calculator<Device, T> device_calc{{.first_level = 0}};
    device_calc.compute_manual(epsilon, WF_STD::span<const T>(d_ref, REF_SIZE), max_dc);
    auto device_approx = device_calc.get_approximator();

    std::vector<T> test_dcs = {
        {0.0001, 0.0001}, {-0.0001, 0.0001}, {0.0001, -0.0001},
        {-0.0001, -0.0001}, {0.0, 0.0}, {0.0005, 0.0},
    };
    unsigned num_dcs = test_dcs.size();

    T* d_dcs = nullptr;
    Complex<float>* d_z_out = nullptr;
    unsigned* d_n_out = nullptr;
    unsigned* d_skipped_out = nullptr;
    CUDA_CHECK(cudaMallocManaged(&d_dcs, num_dcs * sizeof(T)));
    CUDA_CHECK(cudaMallocManaged(&d_z_out, num_dcs * sizeof(Complex<float>)));
    CUDA_CHECK(cudaMallocManaged(&d_n_out, num_dcs * sizeof(unsigned)));
    CUDA_CHECK(cudaMallocManaged(&d_skipped_out, num_dcs * sizeof(unsigned)));
    std::memcpy(d_dcs, test_dcs.data(), num_dcs * sizeof(T));

    std::vector<Complex<float>> host_z(num_dcs);
    std::vector<unsigned> host_n(num_dcs);
    std::vector<unsigned> host_skipped(num_dcs);
    for (unsigned i = 0; i < num_dcs; ++i) {
        auto [z, n, skipped] = bla::escape_approximate(
            test_dcs[i], WF_STD::span<const T>(ref.data(), ref.size()), REF_SIZE, 2.0, host_approx);
        host_z[i] = z;
        host_n[i] = n;
        host_skipped[i] = skipped;
    }

    int blocks = (num_dcs + 255) / 256;
    escape_kernel<<<blocks, 256>>>(d_dcs, num_dcs, d_ref, REF_SIZE, 2.0,
                                   device_approx, d_z_out, d_n_out, d_skipped_out);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    bool ok = true;
    for (unsigned i = 0; i < num_dcs; ++i) {
        if (host_n[i] != d_n_out[i]) {
            std::cerr << "  DC[" << i << "] escape count: host=" << host_n[i]
                      << " device=" << d_n_out[i] << std::endl;
            ok = false;
        }
        if (host_skipped[i] != d_skipped_out[i]) {
            std::cerr << "  DC[" << i << "] skipped: host=" << host_skipped[i]
                      << " device=" << d_skipped_out[i] << std::endl;
            ok = false;
        }
        float z_diff = std::abs(host_z[i].real() - d_z_out[i].real())
                      + std::abs(host_z[i].imag() - d_z_out[i].imag());
        if (z_diff > Z_TOL) {
            std::cerr << "  DC[" << i << "] z: host=(" << host_z[i].real()
                      << "," << host_z[i].imag() << ") device=(" << d_z_out[i].real()
                      << "," << d_z_out[i].imag() << ")" << std::endl;
            ok = false;
        }
    }

    CUDA_CHECK(cudaFree(d_ref));
    CUDA_CHECK(cudaFree(d_dcs));
    CUDA_CHECK(cudaFree(d_z_out));
    CUDA_CHECK(cudaFree(d_n_out));
    CUDA_CHECK(cudaFree(d_skipped_out));
    return ok;
}

auto test_compute_search() -> bool {
    constexpr unsigned REF_SIZE = 100;
    auto ref = make_reference(REF_SIZE);
    auto probes = make_probes();
    auto max_dc = T{0.001, 0.001};
    bla::Params params{
        .first_level = 0,
        .lower_exp = -10.0,
        .upper_exp = 0.0,
        .tolerance = 1e-4,
        .convergence_radius = 0.1,
    };

    bla::Calculator<Host, T> host_calc{params};
    host_calc.compute_search(WF_STD::span<const T>(probes.data(), probes.size()),
                             max_dc, WF_STD::span<const T>(ref.data(), ref.size()));
    auto host_approx = host_calc.get_approximator();

    T* d_ref = nullptr;
    T* d_probes = nullptr;
    CUDA_CHECK(cudaMallocManaged(&d_ref, REF_SIZE * sizeof(T)));
    CUDA_CHECK(cudaMallocManaged(&d_probes, probes.size() * sizeof(T)));
    std::memcpy(d_ref, ref.data(), REF_SIZE * sizeof(T));
    std::memcpy(d_probes, probes.data(), probes.size() * sizeof(T));

    bla::Calculator<Device, T> device_calc{params};
    device_calc.compute_search(WF_STD::span<const T>(d_probes, probes.size()),
                               max_dc, WF_STD::span<const T>(d_ref, REF_SIZE));
    auto device_approx = device_calc.get_approximator();

    bool ok = true;

    if (host_approx.columns.size() != device_approx.columns.size()) {
        std::cerr << "  Column count mismatch: " << host_approx.columns.size()
                  << " vs " << device_approx.columns.size() << std::endl;
        ok = false;
    } else {
        for (std::size_t i = 0; i < host_approx.columns.size(); ++i) {
            if (!columns_equal(host_approx.columns[i], device_approx.columns[i])) {
                std::cerr << "  Column[" << i << "]: host=(" << host_approx.columns[i].start
                          << "," << host_approx.columns[i].count << ") device=("
                          << device_approx.columns[i].start << "," << device_approx.columns[i].count
                          << ")" << std::endl;
                ok = false;
            }
        }
    }

    if (host_approx.approximations.size() != device_approx.approximations.size()) {
        std::cerr << "  Approximation count mismatch: " << host_approx.approximations.size()
                  << " vs " << device_approx.approximations.size() << std::endl;
        ok = false;
    } else {
        for (std::size_t i = 0; i < host_approx.approximations.size(); ++i) {
            if (!bla_equal(host_approx.approximations[i], device_approx.approximations[i])) {
                auto& a = host_approx.approximations[i];
                auto& b = device_approx.approximations[i];
                std::cerr << "  Approx[" << i
                          << "]: host a=(" << a.a.real() << "," << a.a.imag()
                          << ") b=(" << a.b.real() << "," << a.b.imag() << ") r=" << a.r
                          << " device a=(" << b.a.real() << "," << b.a.imag()
                          << ") b=(" << b.b.real() << "," << b.b.imag() << ") r=" << b.r
                          << std::endl;
                ok = false;
            }
        }
    }

    CUDA_CHECK(cudaFree(d_ref));
    CUDA_CHECK(cudaFree(d_probes));
    return ok;
}

} // anonymous namespace

int main() {
    int deviceCount = 0;
    cudaError_t cudaStatus = cudaGetDeviceCount(&deviceCount);
    if (cudaStatus != cudaSuccess) {
        std::cerr << "cudaGetDeviceCount failed: " << cudaGetErrorString(cudaStatus) << std::endl;
        return 1;
    }
    if (deviceCount == 0) {
        std::cerr << "No CUDA devices found" << std::endl;
        return 1;
    }
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    std::cerr << "Using GPU: " << prop.name
              << " (compute " << prop.major << "." << prop.minor << ")" << std::endl;

    int failures = 0;
    auto run = [&](const char* name, auto fn) {
        std::cout << name << " ... " << std::flush;
        bool ok = fn();
        std::cout << (ok ? "PASSED" : "FAILED") << std::endl;
        if (!ok) failures++;
    };

    std::cout << "=== bla_test ===" << std::endl;

    run("Test 1: compute_manual columns+approx (first_level=0)",
        []{ return test_compute_manual(100, 0); });
    run("Test 2: compute_manual columns+approx (first_level=1)",
        []{ return test_compute_manual(100, 1); });
    run("Test 3: escape_approximate", test_escape_approximate);
    run("Test 4: compute_search", test_compute_search);

    std::cout << std::endl;
    if (failures == 0)
        std::cout << "All tests passed." << std::endl;
    else
        std::cout << failures << " test(s) failed." << std::endl;

    return failures == 0 ? 0 : 1;
}
