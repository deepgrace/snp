//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#ifndef ASYNC_CONNECT_HPP
#define ASYNC_CONNECT_HPP

#include <boost/asio.hpp>
#include <unifex/receiver_concepts.hpp>

namespace snp
{
    namespace net = boost::asio;

    template <typename Stream, typename Endpoints>
    struct async_connect
    {
        using error_code_t = boost::system::error_code;
        using endpoint_t = typename Stream::protocol_type::endpoint;

        template <template <typename ...> typename Variant, template <typename ...> typename Tuple>
        using value_types = Variant<Tuple<endpoint_t>>;

        template <template <typename ...> typename Variant>
        using error_types = Variant<error_code_t>;

        static constexpr bool sends_done = true;

        async_connect(Stream& stream, Endpoints&& endpoints) : stream(stream), endpoints(std::forward<Endpoints>(endpoints))
        {
        }

        template <typename Receiver>
        struct operation
        {
            constexpr decltype(auto) start() noexcept
            {
                net::async_connect(stream, endpoints, [receiver = std::move(receiver)](error_code_t ec, endpoint_t ep) mutable
                {
                    if (!ec)
                        unifex::set_value(std::move(receiver), ep);
                    else
                        unifex::set_error(std::move(receiver), ec);
                });
            }

            Receiver receiver;

            Stream& stream;
            Endpoints endpoints;
        };

        template <typename Receiver>
        constexpr decltype(auto) connect(Receiver&& receiver)
        {
            return operation<std::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver), stream, endpoints};
        }

        Stream& stream;
        Endpoints endpoints;
    };

    template <typename Stream, typename Endpoints>
    async_connect(Stream& stream, Endpoints&& endpoints) -> async_connect<Stream, Endpoints>;
}

#endif
