
#include <concepts>
#include <vector>
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
    std::vector<stored_type> value;

    auto size() const -> std::size_t {
        return value.size();
    }
    auto resize(std::size_t new_size) -> void {
        value.resize(new_size);
    }
    T& operator[](std::size_t i) {
        return value[i];
    }
    const T& operator[](std::size_t i) const {
        return value[i];
    }
    View as_view() { // NOTE: Unused
        return std::span(value);
    }
    ConstView as_view() const { // NOTE: Unused
        return std::span(value);
    }
    View get_view() {
        return std::span(value);
    }
    ConstView get_view() const {
        return std::span(value);
    }
    void swap(HostBuffer& other) {
        value.swap(other.value);
    }
};

// TODO: DeviceBuffer implementation

};
