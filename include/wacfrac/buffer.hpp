#pragma once

#include <concepts>
#include <memory>
#include <span>

namespace wacfrac {

template <template<typename>typename B>
concept BufferLike = requires(B<int> buff, B<int> other){
    { buff.size() } -> std::same_as<std::size_t>;
    { buff.resize(std::size_t{}) } -> std::same_as<void>;
    { buff[std::size_t{}] } -> std::same_as<int&>;
    { buff.get_view() } -> std::same_as<typename B<int>::View>;
    { buff.swap(other) } -> std::same_as<void>;
    // TODO: Can copy from std::span
    // TODO: Can initialize with a given size
};

template <typename T>
struct HostBuffer {
    using stored_type = std::remove_cvref_t<T>;
    using View = std::span<T>;
    using ConstView = std::span<const stored_type>;
    std::unique_ptr<stored_type[]> data;
    std::size_t len = 0;

    auto size() const -> std::size_t {
        return len;
    }
    auto resize(std::size_t new_size) -> void {
        data = std::make_unique<stored_type[]>(new_size);
        len = new_size;
    }
    T& operator[](std::size_t i) {
        return data[i];
    }
    const T& operator[](std::size_t i) const {
        return data[i];
    }
    View as_view() { // NOTE: Unused
        return View(data.get(), len);
    }
    ConstView as_view() const { // NOTE: Unused
        return ConstView(data.get(), len);
    }
    View get_view() {
        return View(data.get(), len);
    }
    ConstView get_view() const {
        return ConstView(data.get(), len);
    }
    void swap(HostBuffer& other) {
        std::swap(data, other.data);
        std::swap(len, other.len);
    }
};

// TODO: DeviceBuffer implementation

};
