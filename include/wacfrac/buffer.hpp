#pragma once
#include "wacfrac/log.hpp"
#include <sycl/detail/common.hpp>
#include <sycl/sycl.hpp>
#include <cstddef>
#include <memory>
#include <sycl/usm.hpp>
#include <sycl/usm/usm_enums.hpp>
#include <utility>
#include <span>

namespace wacfrac {

template<typename T>
class Buffer {
    private:
    std::unique_ptr<T[]> _data;
    std::size_t _size;

    public:
    Buffer() : _data(nullptr), _size(0) {}
    Buffer(T* raw, std::size_t size)
        : _data(raw)
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
    auto as_span() const { return std::span<T>(_data.get(), _size); }
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
    operator std::span<T>() const requires (!std::is_const_v<T>) { return {_data.get(), _size}; }
    operator std::span<const T>() const { return {_data.get(), _size}; }

    friend void swap(Buffer& lhs, Buffer& rhs) {
        std::swap(lhs._data, rhs._data);
        std::swap(lhs._size, rhs._size);
    }
};

struct Arena {
    std::span<std::byte> memory;

    Arena(const Arena&) = default;
    template<typename T>
    Arena(std::span<T> memory): memory{reinterpret_cast<std::byte*>(memory.data()), memory.size_bytes()} {};

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
    auto alloc(std::size_t count) -> std::span<T> {
        return {reinterpret_cast<T*>(alloc_bytes(sizeof(T) * count)), count};
    }
};

template<typename T>
class DeviceBuffer {
    public:
    DeviceBuffer() = default;
    DeviceBuffer(sycl::queue& q, std::size_t count, sycl::usm::alloc kind = sycl::usm::alloc::shared)
        : _queue{&q}
        , _data(sycl::malloc<T>(count, q, kind))
        , _size(count)
    {}
    DeviceBuffer(sycl::queue& q, std::span<const T> data, sycl::usm::alloc kind = sycl::usm::alloc::shared)
        : _queue{&q}
        , _data(sycl::malloc<T>(data.size(), q, kind))
        , _size(data.size())
    {
        q.memcpy(_data, data.data(), data.size_bytes());
    }
    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;
    DeviceBuffer(DeviceBuffer&& other)
        : _queue(std::exchange(other._queue, nullptr))
        , _data(std::exchange(other._data, nullptr))
        , _size(std::exchange(other._size, 0)) 
    {}
    DeviceBuffer& operator= (DeviceBuffer&& other){
        _queue = std::exchange(other._queue, nullptr);
        _data = std::exchange(other._data, nullptr);
        _size = std::exchange(other._size, 0);
        return *this;
    }
    ~DeviceBuffer() {
        if (_data && _queue) {
            sycl::free(_data, *_queue);
        }
    }
    auto as_span() const { return std::span<T>(_data, _size); }
    auto data() const {
        return _data;
    }
    auto size() const {
        return _size;
    }
    auto get_queue() {
        return _queue;
    }
    decltype(auto) operator[](std::size_t idx) const {
        return _data[idx]; // WARN: Permits out of bounds access with no exception
    }
    decltype(auto) operator[](std::size_t idx) {
        return _data[idx];
    }
    operator std::span<T>() const requires (!std::is_const_v<T>) { return {_data, _size}; }
    operator std::span<const T>() const { return {_data, _size}; }
    friend void swap(DeviceBuffer& lhs, DeviceBuffer& rhs) {
        std::swap(lhs._queue, rhs._queue);
        std::swap(lhs._data, rhs._data);
        std::swap(lhs._size, rhs._size);
    }

    private:
    sycl::queue* _queue = nullptr;
    T* _data = nullptr; // NOTE: Raw pointer. No unique_ptr with a custom deleter for this guy; I'm a rebel.
    std::size_t _size = 0;
};

class DeviceArena {
    public:
    DeviceArena() = default;
    DeviceArena(sycl::queue& q, std::size_t size, sycl::usm::alloc kind = sycl::usm::alloc::shared)
        : _queue{&q}
        , _data{static_cast<std::byte*>(sycl::malloc(size, q, kind))}
        , _capacity{size}
        , _used{0}
    {}
    ~DeviceArena() {
        if (_data && _queue) {
            sycl::free(_data, *_queue);
        }
    }
    DeviceArena(DeviceArena&) = delete;
    DeviceArena(DeviceArena&& other)
        : _queue{std::exchange(other._queue, nullptr)}
        , _data{std::exchange(other._data, nullptr)}
        , _capacity{std::exchange(other._capacity, 0)}
        , _used{std::exchange(other._used, 0)}
    {}
    void reset() { _used = 0; }
    auto capacity() const { return _capacity; }
    auto used() const { return _used; }
    auto remaining() const { return _capacity - _used; }

    auto allocate_bytes(std::size_t bytes, std::size_t alignment) -> std::byte* {
        std::size_t aligned_offset = align_up(_used, alignment);
        if (aligned_offset + bytes > _capacity) {
            logging::error("Arena allocation failed, {} bytes requested", bytes);
            return nullptr;
        }
 
        auto ptr {_data + aligned_offset};
        _used = aligned_offset + bytes;

        logging::debug("Arena allocated {}, {} remaining", bytes, remaining());
        return ptr;
    }

    template <typename T>
    auto allocate() -> T* {
        return reinterpret_cast<T*>(allocate_bytes(sizeof(T), alignof(T)));
    }

    template <typename T>
    auto allocate(std::size_t count) -> std::span<T> {
        return {reinterpret_cast<T*>(allocate_bytes(count * sizeof(T), alignof(T))), count};
    }
 
    private:
    static auto align_up(std::size_t n, std::size_t alignment) -> std::size_t {
        return (n + alignment - 1) & ~(alignment - 1);
    }

    sycl::queue* _queue = nullptr;
    std::byte* _data = nullptr;
    std::size_t _capacity = 0;
    std::size_t _used = 0;
};

} //namespace wacfrac
