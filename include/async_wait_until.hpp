//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#ifndef ASYNC_WAIT_UNTIL_HPP
#define ASYNC_WAIT_UNTIL_HPP

#include <boost/asio.hpp>
#include <unifex/receiver_concepts.hpp>

namespace snp
{
    namespace net = boost::asio;

    template <typename Timer>
    struct async_wait_until
    {
        template <typename T, typename = std::void_t<>>
        struct impl : std::type_identity<typename T::time_point>
        {
        };

        template <typename T>
        struct impl<T, std::void_t<typename T::time_type>> : std::type_identity<typename T::time_type>
        {
        };

        using time_point = typename impl<Timer>::type;
        using error_code_t = boost::system::error_code;

        template <template <typename ...> typename Variant, template <typename ...> typename Tuple>
        using value_types = Variant<Tuple<>>;

        template <template <typename ...> typename Variant>
        using error_types = Variant<error_code_t>;

        static constexpr bool sends_done = true;

        async_wait_until(Timer& timer, const time_point& tp) : timer(timer), tp(tp)
        {
        }

        template <typename Receiver>
        struct operation
        {
            constexpr decltype(auto) start() noexcept
            {
                timer.expires_at(tp);

                timer.async_wait([receiver = std::move(receiver)](error_code_t ec) mutable
                {
                    if (ec == net::error::operation_aborted)
                        unifex::set_done(std::move(receiver));
                    else
                        unifex::set_value(std::move(receiver));
                });
            }

            Receiver receiver;

            Timer& timer;
            time_point tp;
        };

        template <typename Receiver>
        constexpr decltype(auto) connect(Receiver&& receiver)
        {
            return operation<std::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver), timer, tp};
        }

        Timer& timer;
        time_point tp;
    };
}

#endif
