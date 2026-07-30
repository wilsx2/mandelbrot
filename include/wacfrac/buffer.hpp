#pragma once
#include "wacfrac/log.hpp"

#include <cassert>
#include <cstddef>
#include <span>
#include <sycl/detail/common.hpp>
#include <sycl/sycl.hpp>
#include <utility>

namespace wacfrac
{

    template <typename T> class DeviceBuffer
    {
    public:
        DeviceBuffer() = default;
        DeviceBuffer(sycl::queue& q, std::size_t count, sycl::usm::alloc kind = sycl::usm::alloc::shared)
            : _queue{q},
              _data(sycl::malloc<T>(count, q, kind)),
              _size(count)
        {
        }
        DeviceBuffer(sycl::queue& q, std::span<const T> data, sycl::usm::alloc kind = sycl::usm::alloc::shared)
            : _queue{q},
              _data(sycl::malloc<T>(data.size(), q, kind)),
              _size(data.size())
        {
            q.memcpy(_data, data.data(), data.size_bytes());
        }
        DeviceBuffer(const DeviceBuffer&) = delete;
        DeviceBuffer& operator=(const DeviceBuffer&) = delete;
        DeviceBuffer(DeviceBuffer&& other)
            : _queue(std::move(other._queue)),
              _data(std::exchange(other._data, nullptr)),
              _size(std::exchange(other._size, 0))
        {
        }
        DeviceBuffer& operator=(DeviceBuffer&& other)
        {
            if (_data)
                sycl::free(_data, _queue);
            _queue = std::move(other._queue);
            _data = std::exchange(other._data, nullptr);
            _size = std::exchange(other._size, 0);
            return *this;
        }
        ~DeviceBuffer()
        {
            if (_data)
                sycl::free(_data, _queue);
        }
        auto as_span() const { return std::span<T>(_data, _size); }
        auto data() const { return _data; }
        auto size() const { return _size; }
        auto& queue() { return _queue; }
        decltype(auto) operator[](std::size_t idx) const
        {
            assert(idx < _size);
            return _data[idx];
        }
        decltype(auto) operator[](std::size_t idx)
        {
            assert(idx < _size);
            return _data[idx];
        }
        operator std::span<T>() const
            requires(!std::is_const_v<T>)
        {
            return {_data, _size};
        }
        operator std::span<const T>() const { return {_data, _size}; }
        friend void swap(DeviceBuffer& lhs, DeviceBuffer& rhs)
        {
            std::swap(lhs._queue, rhs._queue);
            std::swap(lhs._data, rhs._data);
            std::swap(lhs._size, rhs._size);
        }

    private:
        sycl::queue _queue{};
        T* _data = nullptr; // NOTE: Raw pointer. No unique_ptr with a custom deleter for this guy; I'm a rebel.
        std::size_t _size = 0;
    };

    class DeviceArena
    {
    public:
        DeviceArena() = default;
        DeviceArena(sycl::queue& q, std::size_t size, sycl::usm::alloc kind = sycl::usm::alloc::shared)
            : _queue{q},
              _data{static_cast<std::byte*>(sycl::malloc(size, q, kind))},
              _capacity{size},
              _used{0},
              _kind{kind}
        {
        }
        ~DeviceArena()
        {
            if (_data)
                sycl::free(_data, _queue);
        }
        DeviceArena(DeviceArena&) = delete;
        DeviceArena(DeviceArena&& other)
            : _queue{std::move(other._queue)},
              _data{std::exchange(other._data, nullptr)},
              _capacity{std::exchange(other._capacity, 0)},
              _used{std::exchange(other._used, 0)},
              _kind{other._kind}
        {
        }
        void reset() { _used = 0; }
        void grow(std::size_t new_capacity)
        {
            if (new_capacity <= _capacity)
                return;
            if (_data)
                sycl::free(_data, _queue);
            _data = static_cast<std::byte*>(sycl::malloc(new_capacity, _queue, _kind));
            _capacity = new_capacity;
            _used = 0;
        }

        auto allocate_bytes(std::size_t bytes, std::size_t alignment) -> std::byte*
        {
            std::size_t aligned_offset = align_up(_used, alignment);
            if (aligned_offset + bytes > _capacity)
            {
                logging::error("Arena allocation failed, {} bytes requested", bytes);
                return nullptr;
            }

            auto ptr{_data + aligned_offset};
            _used = aligned_offset + bytes;

            logging::debug("Arena allocated {}, {} remaining", bytes, _capacity - _used);
            return ptr;
        }

        template <typename T> auto allocate() -> T*
        {
            return reinterpret_cast<T*>(allocate_bytes(sizeof(T), alignof(T)));
        }

        template <typename T> auto allocate(std::size_t count) -> std::span<T>
        {
            return {reinterpret_cast<T*>(allocate_bytes(count * sizeof(T), alignof(T))), count};
        }

    private:
        static auto align_up(std::size_t n, std::size_t alignment) -> std::size_t
        {
            return (n + alignment - 1) & ~(alignment - 1);
        }

        sycl::queue _queue{};
        std::byte* _data = nullptr;
        std::size_t _capacity = 0;
        std::size_t _used = 0;
        sycl::usm::alloc _kind = sycl::usm::alloc::shared;
    };

} // namespace wacfrac
