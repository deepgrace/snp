//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#ifndef ASYNC_ACCEPT_HPP
#define ASYNC_ACCEPT_HPP

#include <boost/asio.hpp>
#include <unifex/receiver_concepts.hpp>

namespace snp
{
    namespace net = boost::asio;

    template <typename Acceptor>
    struct async_accept
    {
        using error_code_t = boost::system::error_code;

        template <template <typename ...> typename Tuple, typename T, typename = std::void_t<>>
        struct impl : std::type_identity<Tuple<>>
        {
        };

        template <template <typename ...> typename Tuple, typename T>
        struct impl<Tuple, T, std::void_t<typename T::protocol_type::socket>>
        {
            using type = Tuple<typename T::protocol_type::socket>;
        };

        template <template <typename ...> typename Variant, template <typename ...> typename Tuple>
        using value_types = Variant<typename impl<Tuple, Acceptor>::type>;

        template <template <typename ...> typename Variant>
        using error_types = Variant<error_code_t>;

        static constexpr bool sends_done = true;

        async_accept(Acceptor& acceptor) : acceptor(acceptor)
        {
        }

        template <typename Receiver>
        struct operation
        {
            constexpr decltype(auto) start() noexcept
            {
                acceptor.async_accept([receiver = std::move(receiver)](error_code_t ec, auto... socket) mutable
                {
                    if (!ec)
                        unifex::set_value(std::move(receiver), std::move(socket)...);
                    else
                        unifex::set_error(std::move(receiver), ec);
                });
            }

            Receiver receiver;
            Acceptor& acceptor;
        };

        template <typename Receiver>
        constexpr decltype(auto) connect(Receiver&& receiver)
        {
            return operation<std::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver), acceptor};
        }

        Acceptor& acceptor;
    };
}

#endif
