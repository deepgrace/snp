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

    template <typename T>
    using timer_t = net::basic_waitable_timer<typename T::clock>;

    template <typename T, typename U>
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
                if constexpr(requires { typename T::clock; })
                    u.expires_at(t);
                else if constexpr(requires { std::declval<T>().zero(); })
                    u.expires_after(t);

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
            return schedule_sender<bool, bool>(*ioc);
        }

        template <typename T>
        constexpr decltype(auto) schedule_at(const T& time_point) const noexcept
        {
            return schedule_sender<T, timer_t<T>>(*ioc, time_point, timer_t<T>(*ioc));
        }

        template <typename T>
        constexpr decltype(auto) schedule_after(const T& duration) const noexcept
        {
            return schedule_sender<T, net::steady_timer>(*ioc, duration, net::steady_timer(*ioc));
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

        net::io_context ioc;
    };
}

namespace snp
{
    using asio_context = snp::asio::context;
    using asio_scheduler = snp::asio::scheduler;
}

#endif
