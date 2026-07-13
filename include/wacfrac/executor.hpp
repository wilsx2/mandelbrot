#include <concepts>
#include <ranges>
#include <algorithm>
#include <execution>

namespace wacfrac {

template<typename T>
concept ExecutorLike = requires (const T& a) {
    { a(std::size_t{}, [](unsigned tid){ (void) tid; }) } -> std::same_as<void>;
};

struct SequentialExecutor {
    template<typename F>
    void operator()(std::size_t count, F&& func) const {
        for (std::size_t i = 0; i < count; ++i) {
            func(i);
        }
    }
};

struct ParallelExecutor {
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
