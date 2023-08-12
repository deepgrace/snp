//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#ifndef ASYNC_WAIT_HPP
#define ASYNC_WAIT_HPP

#include <boost/asio.hpp>
#include <unifex/receiver_concepts.hpp>

namespace snp
{
    namespace net = boost::asio;

    template <typename Timer>
    struct async_wait
    {
        template <typename T, typename = std::void_t<>>
        struct impl : std::type_identity<typename T::duration>
        {
        };

        template <typename T>
        struct impl<T, std::void_t<typename T::duration_type>> : std::type_identity<typename T::duration_type>
        {
        };

        using duration = typename impl<Timer>::type;
        using error_code_t = boost::system::error_code;

        template <template <typename ...> typename Variant, template <typename ...> typename Tuple>
        using value_types = Variant<Tuple<>>;

        template <template <typename ...> typename Variant>
        using error_types = Variant<error_code_t>;

        static constexpr bool sends_done = true;

        async_wait(Timer& timer, const duration& dur) : timer(timer), dur(dur)
        {
        }

        template <typename Receiver>
        struct operation
        {
            constexpr decltype(auto) start() noexcept
            {
                timer.expires_from_now(dur);

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
            duration dur;
        };

        template <typename Receiver>
        constexpr decltype(auto) connect(Receiver&& receiver)
        {
            return operation<std::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver), timer, dur};
        }

        Timer& timer;
        duration dur;
    };
}

#endif
