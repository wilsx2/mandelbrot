#pragma once

#include "wacfrac/buffer.hpp"
#include <concepts>
#include <ranges>
#include <algorithm>
#include <execution>

namespace wacfrac {

template<typename T>
concept ExecutorLike = requires (const T& a) {
    requires BufferLike<T::template Buffer>;
    { a(std::size_t{}, [](unsigned tid){ (void) tid; }) } -> std::same_as<void>;
};

struct SequentialExecutor {
    template<typename T>
    using Buffer = HostBuffer<T>;

    template<typename F>
    void operator()(std::size_t count, F&& func) const {
        for (std::size_t i = 0; i < count; ++i) {
            func(i);
        }
    }
};

struct ParallelExecutor {
    template<typename T>
    using Buffer = HostBuffer<T>;

    template<typename F>
    void operator()(std::size_t count, F&& func) const {
        auto range {std::ranges::iota_view(0uz, count)};
        std::for_each(
            std::execution::par_unseq,
            range.begin(),
            range.end(),
            func);
    }
};

}
