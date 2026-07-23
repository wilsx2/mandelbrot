#pragma once
#include "wacfrac/buffer.hpp"
#include "wacfrac/log.hpp"
#include <cstddef>
#include <cstring>
#include <memory>
#include <ranges>
#include <algorithm>
#include <execution>
#include <utility>
#include <span>

namespace wacfrac {

class ProcessingContext {
    public:
    template<typename T>
    using Buffer = Buffer<T>;

    template<typename T>
    auto make_buffer(std::size_t count) {
        return Buffer<T>(alloc<T>(count), count);
    }
    template<typename T>
    auto make_buffer(std::span<T> src) {
        auto buf {Buffer<T>(alloc<T>(src.size()), src.size())};
        memcpy<T>(buf.as_span(), src);
        return buf;
    }
    template<typename T>
    auto make_buffer(std::span<const T> src) {
        auto buf {Buffer<T>(alloc<T>(src.size()), src.size())};
        memcpy<T>(buf.as_span(), src);
        return buf;
    }
    template<typename T>
    auto memcpy(std::span<T> dst, std::span<const T> src) -> void {
        std::memcpy(dst.data(), src.data(), dst.size() * sizeof(T));
    }
    template <typename F>
    void parallel_for(std::size_t count, F&& func) const {
        auto range {std::ranges::iota_view(std::size_t{0}, count)};
        std::for_each(
            std::execution::par_unseq,
            range.begin(),
            range.end(),
            func);
    }

    private:
    template<typename T>
    auto alloc(std::size_t count) -> T* {
       return new T[count]();
    }
};

}
