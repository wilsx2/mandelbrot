#pragma once
#include "wacfrac/macros.hpp"
#include "wacfrac/log.hpp"
#include <cstddef>
#include <memory>
#include <utility>

#if defined(__CUDACC__)
#include <cuda/std/span>
#else
#include <span>
#endif

namespace wacfrac {

template<typename T, typename Deleter = ::std::default_delete<T[]>>
class Buffer {
    private:
    std::unique_ptr<T[], Deleter> _data;
    std::size_t _size;

    public:
    Buffer() : _data(nullptr), _size(0) {}
    Buffer(T* raw, std::size_t size, Deleter del = {})
        : _data(raw, del)
        , _size(size)
    {}
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) : _data(std::move(other._data)), _size(std::exchange(other._size, 0)) {}
    Buffer& operator= (Buffer&& other){
        _data = std::move(other._data);
        _size = std::exchange(other._size, 0);
        return *this;
    }
    auto as_span() const { return WF_STD::span<T>(_data.get(), _size); }
    auto data() const {
        return _data.get();
    }
    auto size() const {
        return _size;
    }
    decltype(auto) operator[](std::size_t idx) const {
        return _data.get()[idx]; // NOTE: Permits out of bounds reads
    }
    decltype(auto) operator[](std::size_t idx) {
        return _data.get()[idx]; // NOTE: Permits out of bounds reads
    }
    operator WF_STD::span<T>() const requires (!std::is_const_v<T>) { return {_data.get(), _size}; }
    operator WF_STD::span<const T>() const { return {_data.get(), _size}; }

    friend void swap(Buffer& lhs, Buffer& rhs) {
        WF_STD::swap(lhs._data, rhs._data);
        WF_STD::swap(lhs._size, rhs._size);
    }
};

struct Arena {
    WF_STD::span<std::byte> memory;

    Arena(const Arena&) = default;
    template<typename T>
    Arena(WF_STD::span<T> memory): memory{reinterpret_cast<std::byte*>(memory.data()), memory.size_bytes()} {};

    auto alloc_bytes(std::size_t bytes) -> void* {
        if (memory.size() < bytes) {
            logging::error("Arena allocation failed, {} bytes requested", bytes);
            return nullptr;
        }
        auto ptr {memory.data()};
        memory = {memory.data() + bytes, memory.size() - bytes};
        logging::debug("Arena allocated {}, {} remaining", bytes, memory.size_bytes());
        return ptr;
    }
    template<typename T>
    auto alloc() -> T* {
        return reinterpret_cast<T*>(alloc_bytes(sizeof(T)));
    }
    template<typename T>
    auto alloc(std::size_t count) -> WF_STD::span<T> {
        return {reinterpret_cast<T*>(alloc_bytes(sizeof(T) * count)), count};
    }
};

} //namespace wacfrac
