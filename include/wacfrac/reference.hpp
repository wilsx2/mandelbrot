#pragma once

#include "wacfrac/bla.hpp"
#include "wacfrac/complex_concept.hpp"
#include "wacfrac/log.hpp"
#include "wacfrac/orbit.hpp"
#include "wacfrac/types.hpp"

#include <barrier>
#include <cstddef>
#include <span>
#include <sycl/queue.hpp>
#include <thread>
#include <vector>

namespace wacfrac
{

    template <ComplexConcept T = DoubleComplex>
    auto compute_reference(std::span<T> buffer, MultiComplex c, double escape_radius = 4.0) -> unsigned
    {
        auto max_n{static_cast<unsigned>(buffer.size())};
        logging::debug("Computing reference orbit at ({}) max_n={} escape_radius={}", c, max_n, escape_radius);

        buffer[0] = to_complex<T>(MultiComplex{});

        MultiComplex z{};
        auto n{0u};
        // We cannot use orbit.hpp functions because they are marked SYCL_EXTERNAL
        for (; n < max_n - 1;)
        {
            if (static_cast<double>(norm(z)) > escape_radius * escape_radius)
                break;
            z = z * z + c;
            ++n;
            buffer[n] = to_complex<T>(z);
        }

        logging::debug("Reference orbit computed: {} points (max_n={})", n, max_n);
        return n;
    }

    template <ComplexConcept T = DoubleComplex>
    auto compute_reference_mt(std::span<T> buffer, MultiComplex c, double escape_radius = 4.0) -> std::size_t
    {
        auto max_n{static_cast<unsigned>(buffer.size())};
        logging::debug("Computing reference orbit at ({}) max_n={} escape_radius={} (parallel)", c, max_n,
                       escape_radius);
        if (max_n == 0)
            return {};

        buffer[0] = T{0.0, 0.0};
        using CT = ComplexValueTypeT<T>;
        auto count{compute_reference_iteration(c, max_n, escape_radius,
                                               [&](unsigned n, const MultiFloat& r, const MultiFloat& i) {
                                                   buffer[n] = T{to_real<CT>(r), to_real<CT>(i)};
                                               })};
        logging::debug("Reference orbit computed: {} points (max_n={})", count, max_n);
        return count;
    }

    template <std::invocable<unsigned, const MultiFloat&, const MultiFloat&> F>
    auto compute_reference_iteration(MultiComplex c, unsigned max_n, double escape_radius, F&& store_at_n) -> unsigned
    {
        if (max_n == 0)
            return 0;

        auto n_1{1u};
        MultiComplex z{};
        MultiComplex next_z{};
        auto running{true};
        auto cr{c.real()};
        auto ci{c.imag()};
        MultiFloat computed_re;
        MultiFloat computed_im;

        std::barrier sync(2, [&]() noexcept {
            store_at_n(n_1, computed_re, computed_im);
            ++n_1;
            if (n_1 >= max_n || static_cast<double>(norm(next_z)) > escape_radius * escape_radius)
                running = false;
            else
                std::swap(z, next_z);
        });

        std::jthread real_compute{[&]() {
            while (running)
            {
                computed_re = z.real() * z.real() - z.imag() * z.imag() + cr;
                next_z.real(computed_re);
                sync.arrive_and_wait();
            }
        }};
        std::jthread imag_compute{[&]() {
            while (running)
            {
                computed_im = 2 * z.real() * z.imag() + ci;
                next_z.imag(computed_im);
                sync.arrive_and_wait();
            }
        }};

        real_compute.join();
        imag_compute.join();
        return n_1;
    }

    class ReferenceSet
    {
    public:
        ReferenceSet()
            : _size{0u}
        {
        }
        ReferenceSet(sycl::queue& q, unsigned max_n)
            : _float_ref{q, max_n},
              _double_ref{q, max_n},
              _dexp_ref{q, max_n},
              _size{0u}
        {
        }
        ReferenceSet& operator=(ReferenceSet&& other) = default;

        auto max_n() const -> unsigned { return _float_ref.size(); }

        auto size() const -> unsigned { return _size; }

        template <typename T> auto select() const -> std::span<const T>
        {
            if constexpr (std::is_same_v<T, SingleComplex>)
                return _float_ref.as_span().subspan(0, _size);
            else if constexpr (std::is_same_v<T, DoubleComplex>)
                return _double_ref.as_span().subspan(0, _size);
            else
                return _dexp_ref.as_span().subspan(0, _size);
        }

        template <typename T> auto select() -> std::span<T>
        {
            if constexpr (std::is_same_v<T, SingleComplex>)
                return _float_ref.as_span().subspan(0, _size);
            else if constexpr (std::is_same_v<T, DoubleComplex>)
                return _double_ref.as_span().subspan(0, _size);
            else
                return _dexp_ref.as_span().subspan(0, _size);
        }

        auto compute(MultiComplex c, double escape_radius = 4.0) -> void
        {
            if (max_n() == 0)
                return;

            _float_ref[0] = SingleComplex{0.0f, 0.0f};
            _double_ref[0] = DoubleComplex{0.0, 0.0};
            _dexp_ref[0] = DoubleExpComplex{};
            _size = compute_reference_iteration(
                c, max_n(), escape_radius, [&](unsigned n, const MultiFloat& r, const MultiFloat& i) {
                    _float_ref[n] = SingleComplex{to_real<float>(r), to_real<float>(i)};
                    _double_ref[n] = DoubleComplex{to_real<double>(r), to_real<double>(i)};
                    _dexp_ref[n] = DoubleExpComplex{to_real<DoubleExp>(r), to_real<DoubleExp>(i)};
                });

            logging::debug("All references computed: {} points", _size);
        }

    private:
        DeviceBuffer<SingleComplex> _float_ref;
        DeviceBuffer<DoubleComplex> _double_ref;
        DeviceBuffer<DoubleExpComplex> _dexp_ref;
        unsigned _size;
    };

} // namespace wacfrac
