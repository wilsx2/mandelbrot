#pragma once
#include <cuda/std/span>
#include <cuda_runtime.h>
#include <cuda/buffer>
#include <cuda/devices>
#include <cuda/launch>
#include <cuda/memory>
#include <cuda/memory_pool>
#include <cuda/stream>
#include <cccl/cuda/__container/buffer.h>
#include <cccl/cuda/__device/device_ref.h>
#include <cccl/cuda/__stream/stream.h>
#include "wacfrac/bla.hpp"
#include "wacfrac/orbit.hpp"
#include <vector>
#include <cstddef>
#include <cmath>
#include <bit>

namespace wacfrac::bla {

namespace detail {

template <ComplexConcept T, typename Config>
__global__ void bla_compute_initial_kernel(
    Config config,
    ComplexValueTypeT<T> epsilon,
    cuda::std::span<const T> ref,
    T max_dc,
    cuda::std::span<Bla<T>> working,
    Approximator<cuda::std::span, T> approximator);

template <typename T, typename Config>
__global__ void bla_merge_kernel(
    Config config,
    T max_dc,
    std::size_t current_level,
    std::size_t first_level,
    cuda::std::span<Bla<T>> working,
    Approximator<cuda::std::span, T> approximator);

template <ComplexConcept T, typename Config>
__global__ void bla_escape_time_kernel(
    Config config,
    cuda::std::span<const T> probes,
    cuda::std::span<const T> ref,
    double escape_radius,
    cuda::std::span<unsigned> true_escape_times);

template <ComplexConcept T, typename Config>
__global__ void bla_skipped_kernel(
    Config config,
    cuda::std::span<const T> probes,
    cuda::std::span<const T> ref,
    double escape_radius,
    double tolerance,
    unsigned max_n,
    Approximator<cuda::std::span, T> approximator,
    cuda::std::span<const unsigned> true_escape_times,
    cuda::std::span<unsigned> skipped_count,
    cuda::std::span<unsigned> tolerance_failed);

} // namespace detail

template <typename T>
class DeviceCalculator;
template <typename T>
class DeviceCalculator : public GenericCalculator<DeviceCalculator<T>, cuda::std::span, T> {
    public:
    using Base = GenericCalculator<DeviceCalculator<T>, cuda::std::span, T>;
    using CT = Base::CT;
    template <typename U>
    using View = cuda::std::span<U>;

    private:
    cuda::device_ref                    _device;
    cuda::stream                        _stream;
    cuda::device_buffer<ColumnInfo>     _columns;
    cuda::device_buffer<Bla<T>>         _working_approximations;
    cuda::device_buffer<Bla<T>>         _approximations;
    cuda::device_buffer<unsigned>       _true_escape_times;
    cuda::device_buffer<unsigned>       _skipped_count;
    cuda::device_buffer<unsigned>       _tolerance_failed;

    public:
    DeviceCalculator(int device_id, std::size_t first_level)
        : Base(first_level)
        , _device{device_id}
        , _stream{_device}
        , _columns{_stream, cuda::device_default_memory_pool(_device)}
        , _working_approximations{_stream, cuda::device_default_memory_pool(_device)}
        , _approximations{_stream, cuda::device_default_memory_pool(_device)}
        , _true_escape_times{_stream, cuda::device_default_memory_pool(_device)}
        , _skipped_count{_stream, cuda::device_default_memory_pool(_device)}
        , _tolerance_failed{_stream, cuda::device_default_memory_pool(_device)}
    {}

    auto resize_columns(unsigned size) -> void {
        _columns = cuda::make_buffer<ColumnInfo>(
            _stream, cuda::device_default_memory_pool(_device),
            size, cuda::no_init);
        _working_approximations = cuda::make_buffer<Bla<T>>(
            _stream, cuda::device_default_memory_pool(_device),
            size, cuda::no_init);
    }

    auto resize_approximations(unsigned size) -> void {
        _approximations = cuda::make_buffer<Bla<T>>(
            _stream, cuda::device_default_memory_pool(_device),
            size, cuda::no_init);
    }

    auto compute_manual(CT epsilon, View<const T> ref, T max_dc) -> Approximator<cuda::std::span, T> {
        if (ref.size() > this->_max_ref_size) {
            this->_max_ref_size = ref.size();
            this->_last_level = ref.size() < 3
                ? std::size_t{0}
                : static_cast<std::size_t>(std::log2(static_cast<double>(ref.size())));
            auto max_n = ref.size() - 1;

            std::vector<ColumnInfo> host_columns(max_n);
            host_columns[max_n - 1] = {0, 0};
            std::size_t approx_count = 0;
            for (auto m = 1ull; m < max_n; ++m) {
                auto cz = static_cast<unsigned>(std::countr_zero(m - 1));
                auto size = cz >= this->_first_level
                    ? 1 + std::min(cz - this->_first_level, this->_last_level - this->_first_level)
                    : 0ull;
                host_columns[m - 1] = {approx_count, size};
                approx_count += size;
            }

            _columns = cuda::make_buffer<ColumnInfo>(
                _stream, cuda::device_default_memory_pool(_device),
                host_columns.begin(), host_columns.end());
            _working_approximations = cuda::make_buffer<Bla<T>>(
                _stream, cuda::device_default_memory_pool(_device),
                max_n, cuda::no_init);
            resize_approximations(approx_count);
        }

        auto level_size = ref.size() - 2;
        compute_initial_approximations(epsilon, ref, max_dc);
        for (auto i = 1ull; level_size >= 2; ++i) {
            auto even_size = level_size & ~1ull;
            merge_approximations(i, even_size, max_dc);
            level_size /= 2;
        }
        return this->get_approximator();
    }

    auto compute_initial_approximations(CT epsilon, View<const T> ref, T max_dc) -> void {
        if (ref.size() < 3) return;
        auto n = ref.size() - 2;
        auto config = cuda::distribute<256>(n);
        cuda::launch(_stream, config,
            detail::bla_compute_initial_kernel<T, decltype(config)>,
            config, epsilon, ref, max_dc, View<Bla<T>>{_working_approximations}, this->get_approximator());
        _stream.sync();
    }

    auto merge_approximations(std::size_t current_level, std::size_t level_size, T max_dc) -> void {
        auto num_merges = level_size / 2;
        if (num_merges == 0) return;
        auto config = cuda::distribute<256>(num_merges);
        cuda::launch(_stream, config,
            detail::bla_merge_kernel<T, decltype(config)>,
            config, max_dc, current_level, this->_first_level,
            View<Bla<T>>{_working_approximations}, this->get_approximator());
        _stream.sync();
    }

    auto compute_probe_escape_time(View<const T> probes, View<const T> ref, double escape_radius) -> void {
        auto n = probes.size();
        if (n == 0) return;
        if (n > _true_escape_times.size()) {
            _true_escape_times = cuda::make_buffer<unsigned>(
                _stream, cuda::device_default_memory_pool(_device), n, cuda::no_init);
        }
        auto config = cuda::distribute<256>(n);
        cuda::launch(_stream, config,
            detail::bla_escape_time_kernel<T, decltype(config)>,
            config, probes, ref, escape_radius,
            View<unsigned>{_true_escape_times});
        _stream.sync();
    }

    auto compute_skipped_iterations(View<const T> probes, View<const T> ref,
                                     double escape_radius, double tolerance) -> std::optional<unsigned> {
        auto n = probes.size();
        if (n == 0) return 0u;

        if (n > _true_escape_times.size()) {
            _true_escape_times = cuda::make_buffer<unsigned>(
                _stream, cuda::device_default_memory_pool(_device), n, cuda::no_init);
        }
        if (_skipped_count.size() < 1) {
            _skipped_count = cuda::make_buffer<unsigned>(
                _stream, cuda::device_default_memory_pool(_device), 1, cuda::no_init);
        }
        if (_tolerance_failed.size() < 1) {
            _tolerance_failed = cuda::make_buffer<unsigned>(
                _stream, cuda::device_default_memory_pool(_device), 1, cuda::no_init);
        }

        cudaMemsetAsync(_skipped_count.data(), 0, sizeof(unsigned), _stream.get());
        cudaMemsetAsync(_tolerance_failed.data(), 0, sizeof(unsigned), _stream.get());

        auto approximator = this->get_approximator();

        auto config = cuda::distribute<256>(n);
        cuda::launch(_stream, config,
            detail::bla_skipped_kernel<T, decltype(config)>,
            config, probes, ref, escape_radius, tolerance,
            static_cast<unsigned>(ref.size()), approximator,
            View<const unsigned>{_true_escape_times},
            View<unsigned>{_skipped_count},
            View<unsigned>{_tolerance_failed});
        _stream.sync();

        unsigned h_failed = 0;
        unsigned h_skipped = 0;
        cudaMemcpy(&h_failed, _tolerance_failed.data(), sizeof(unsigned), cudaMemcpyDeviceToHost);
        cudaMemcpy(&h_skipped, _skipped_count.data(), sizeof(unsigned), cudaMemcpyDeviceToHost);

        if (h_failed > 0) return std::nullopt;
        return h_skipped;
    }

    auto get_columns() -> View<ColumnInfo> { return _columns; }
    auto get_columns() const -> View<const ColumnInfo> { return _columns; }
    auto get_approximations() -> View<Bla<T>> { return _approximations; }
    auto get_approximations() const -> View<const Bla<T>> { return _approximations; }
};

namespace detail {

template <ComplexConcept T, typename Config>
__global__ void bla_compute_initial_kernel(
    Config config,
    ComplexValueTypeT<T> epsilon,
    cuda::std::span<const T> ref,
    T max_dc,
    cuda::std::span<Bla<T>> working,
    Approximator<cuda::std::span, T> approximator)
{
    auto tid = cuda::gpu_thread.rank(cuda::grid, config);
    if (tid + 2 >= ref.size()) return;
    auto m {static_cast<unsigned>(tid + 1)};
    Bla<T> bla {epsilon, ref, max_dc, m, m + 1};
    working[m - 1] = bla;
    if (approximator.first_level == 0) {
        auto* ptr {approximator.approximation_at(m, 0)};
        if (ptr) { *ptr = bla; }
    }
}

template <typename T, typename Config>
__global__ void bla_merge_kernel(
    Config config,
    T max_dc,
    std::size_t current_level,
    std::size_t first_level,
    cuda::std::span<Bla<T>> working,
    Approximator<cuda::std::span, T> approximator)
{
    auto tid = cuda::gpu_thread.rank(cuda::grid, config);
    auto k = tid * 2;
    if (k + 1 >= working.size()) return;

    auto bla = Bla<T>::merge(max_dc, working[k], working[k + 1]);
    working[tid] = bla;

    if (current_level >= approximator.first_level) {
        auto m {1 + (k / 2) * (1ull << current_level)};
        auto* ptr {approximator.approximation_at(m, current_level)};
        if (ptr) { *ptr = bla; }
    }
}

template <ComplexConcept T, typename Config>
__global__ void bla_escape_time_kernel(
    Config config,
    cuda::std::span<const T> probes,
    cuda::std::span<const T> ref,
    double escape_radius,
    cuda::std::span<unsigned> true_escape_times)
{
    auto tid = cuda::gpu_thread.rank(cuda::grid, config);
    if (tid >= probes.size()) return;
    true_escape_times[tid] = escape_perturbed<T>(
        probes[tid], ref, static_cast<unsigned>(ref.size()), escape_radius).second;
}

template <ComplexConcept T, typename Config>
__global__ void bla_skipped_kernel(
    Config config,
    cuda::std::span<const T> probes,
    cuda::std::span<const T> ref,
    double escape_radius,
    double tolerance,
    unsigned max_n,
    Approximator<cuda::std::span, T> approximator,
    cuda::std::span<const unsigned> true_escape_times,
    cuda::std::span<unsigned> skipped_count,
    cuda::std::span<unsigned> tolerance_failed)
{
    auto tid = cuda::gpu_thread.rank(cuda::grid, config);
    if (tid >= probes.size()) return;

    auto [z, approx_escape_time, skipped] = escape_approximate(
        probes[tid], ref, max_n, escape_radius, approximator);

    auto true_time = true_escape_times[tid];
    if (true_time > 0) {
        using std::abs;
        auto ratio = static_cast<double>(approx_escape_time) / static_cast<double>(true_time);
        if (abs(ratio - 1.0) > tolerance) {
            atomicAdd(&tolerance_failed[0], 1u);
            return;
        }
    }
    atomicAdd(&skipped_count[0], skipped);
}

} // namespace detail

} // namespace wacfrac::bla
