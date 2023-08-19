//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#ifndef ASIO_CONTEXT_HPP
#define ASIO_CONTEXT_HPP

#include <chrono>
#include <boost/asio.hpp>
#include <unifex/receiver_concepts.hpp>

namespace snp::asio
{
    namespace net = boost::asio;
    using clock = std::chrono::steady_clock;

    template <typename T, typename = std::void_t<>>
    struct choose : std::type_identity<net::steady_timer>
    {
    };

    template <typename T>
    struct choose<T, std::void_t<typename T::clock>> : std::type_identity<net::basic_waitable_timer<typename T::clock>>
    {
    };

    template <typename T, typename = std::void_t<>>
    struct select : std::type_identity<net::deadline_timer>
    {
    };

    template <typename T>
    struct select<T, std::void_t<typename T::period>> : choose<T>
    {
    };

    template <typename T>
    using timer_t = typename select<T>::type;

    template <typename T, typename U, bool B>
    struct schedule_sender
    {
        using error_code_t = boost::system::error_code;

        template <template <typename ...> typename Variant, template <typename ...> typename Tuple>
        using value_types = Variant<Tuple<>>;

        template <template <typename ...> typename Variant>
        using error_types = Variant<std::exception_ptr>;

        static constexpr bool sends_done = true;

        explicit schedule_sender(net::io_context& ioc, const T& t = {}, U&& u = {}) : ioc(ioc), t(t), u(std::forward<U>(u))
        {
        }

        template <typename Receiver>
        struct operation
        {
            constexpr decltype(auto) start() noexcept
            {
                if constexpr(std::is_same_v<T, bool>)
                    net::post(ioc, [this]{ set_value(); });
                else
                    set_timer();
            }

            void set_value()
            {
                try
                {
                    unifex::set_value(std::move(receiver));
                }
                catch (...)
                {
                    unifex::set_error(std::move(receiver), std::current_exception());
                }
            }

            void set_timer()
            {
                if constexpr(B)
                    u.expires_at(t);
                else
                    u.expires_from_now(t);

                u.async_wait([this](error_code_t ec) mutable
                {
                    if (ec == net::error::operation_aborted)
                        unifex::set_done(std::move(receiver));
                    else
                        set_value();
                });
            }

            Receiver receiver;
            net::io_context& ioc;

            T t;
            U u;
        };

        template <typename Receiver>
        constexpr decltype(auto) connect(Receiver&& receiver)
        {
            return operation<std::remove_cvref_t<Receiver>>(std::forward<Receiver>(receiver), ioc, std::move(t), std::move(u));
        }

        net::io_context& ioc;

        T t;
        U u;
    };

    struct scheduler
    {
        explicit scheduler(net::io_context& ioc) : ioc(&ioc)
        {
        }

        constexpr decltype(auto) now() const noexcept
        {
            return clock::now();
        }

        constexpr decltype(auto) schedule() const noexcept
        {
            return schedule_sender<bool, bool, 1>(*ioc);
        }

        template <typename T>
        constexpr decltype(auto) schedule_at(const T& time_point) const noexcept
        {
            return schedule_sender<T, timer_t<T>, 1>(*ioc, time_point, timer_t<T>(*ioc));
        }

        template <typename T>
        constexpr decltype(auto) schedule_after(const T& duration) const noexcept
        {
            return schedule_sender<T, timer_t<T>, 0>(*ioc, duration, timer_t<T>(*ioc));
        }

        friend constexpr bool operator==(scheduler l, scheduler r) noexcept
        {
            return l.ioc == r.ioc;
        }

        friend constexpr bool operator!=(scheduler l, scheduler r) noexcept
        {
            return l.ioc != r.ioc;
        }

        net::io_context* ioc;
    };

    struct context
    {
        constexpr decltype(auto) get_scheduler() noexcept
        {
            return scheduler(ioc);
        }

        net::io_context& get_io_context() noexcept
        {
            return ioc;
        }

        constexpr decltype(auto) run()
        {
            ioc.run();
        }

        constexpr decltype(auto) stop()
        {
            ioc.stop();
        }

        net::io_context ioc;
    };
}

namespace snp
{
    using asio_context = snp::asio::context;
    using asio_scheduler = snp::asio::scheduler;
}

#endif
