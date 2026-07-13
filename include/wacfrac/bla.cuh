#pragma once
#include "wacfrac/bla.hpp"
#include <cuda/memory>
#include <cccl/cuda/__container/buffer.h>
#include <cccl/cuda/__device/device_ref.h>
#include <cccl/cuda/__stream/stream.h>
#include <cuda/std/span>
#include <cuda/devices>
#include <cuda/stream>
#include <cuda/memory_pool>
#include <cuda/buffer>

namespace wacfrac::bla {

template <typename T>
class DeviceCalculator;
template <typename T>
class DeviceCalculator : public GenericCalculator<DeviceCalculator<T>, cuda::std::span, T> {
    public: 
    using Base = GenericCalculator<DeviceCalculator<T>, cuda::std::span, T>;
    using CT = Base::CT;
    template <typename U>
    using View = std::span<U>;

    protected:
    // TODO: Stream and device params
    cuda::device_buffer<ColumnInfo>     _columns;
    cuda::device_buffer<Bla<T>>         _working_approximations;
    cuda::device_buffer<Bla<T>>         _approximations;
    cuda::device_buffer<unsigned>       _true_escape_times;

    public:
    DeviceCalculator(unsigned device_id, std::size_t first_level) : Base(first_level) {
        // TODO: Use CUDA make buffer and such
    }

    auto resize_columns(unsigned size) -> void {
        // TODO: Use CUDA make buffer and such
    }
    auto resize_approximations(unsigned size) -> void {
        // TODO: Use CUDA make buffer and such
    }

    auto compute_initial_approximations(CT epsilon, View<const T> ref, T max_dc) -> void {
        // TODO: Replace with launch of a kernel lambda
        for (auto m : std::views::iota(1uz, ref.size() - 1)) {
            Bla<T> bla {epsilon, ref, max_dc, static_cast<unsigned>(m), static_cast<unsigned>(m + 1)};
            _working_approximations.at(m - 1) = bla;
            if (0 == Base::_first_level) {
                *(Base::get_approximator().approximation_at(m, 0)) = bla;
            }
        }
    }
    auto merge_approximations(std::size_t current_level, std::size_t level_size, T max_dc) -> void {
        // TODO: Replace with launch of a kernel lambda
        for (auto k : std::views::iota(0uz, level_size) | std::views::stride(2)) {
            auto bla {Bla<T>::merge(max_dc, _working_approximations.at(k), _working_approximations.at(k+1))};
            _working_approximations.at(k/2) = bla;
            if (current_level >= Base::_first_level) {
                auto m {1 + (k / 2) * (1uz << current_level)};
                *(Base::get_approximator().approximation_at(m, current_level)) = bla;
            }
        }
    }
    auto compute_probe_escape_time(View<const T> probes, View<const T> ref, double escape_radius) -> View<const unsigned> {
        // TODO: Replace with launch of a kernel lambda
        _true_escape_times.resize(0);
        _true_escape_times.reserve(probes.size());
        std::ranges::transform(probes, std::back_inserter(_true_escape_times),
            [&ref, &escape_radius](T p) -> unsigned { return escape_perturbed<T>(p, ref, static_cast<unsigned>(ref.size()), escape_radius).second; });
        return _true_escape_times;
    }
    auto get_columns() -> View<ColumnInfo> {
        return _columns;
    }
    auto get_columns() const -> View<const ColumnInfo> {
        return _columns;
    }
    auto get_approximations() -> View<Bla<T>> {
        return _approximations;
    }
    auto get_approximations() const -> View<const Bla<T>> {
        return _approximations;
    }
};

}
