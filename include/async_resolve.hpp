//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#ifndef ASYNC_RESOLVE_HPP
#define ASYNC_RESOLVE_HPP

#include <boost/asio.hpp>
#include <unifex/receiver_concepts.hpp>

namespace snp
{
    namespace net = boost::asio;

    template <typename Context>
    struct async_resolve
    {
        using tcp = net::ip::tcp;

        using error_code_t = boost::system::error_code;
        using results_type = tcp::resolver::results_type;

        template <template <typename ...> typename Variant, template <typename ...> typename Tuple>
        using value_types = Variant<Tuple<results_type>>;

        template <template <typename ...> typename Variant>
        using error_types = Variant<error_code_t>;

        static constexpr bool sends_done = true;

        async_resolve(Context& context, const std::string& host, const std::string& service) : context(context), host(host), service(service)
        {
        }

        template <typename Receiver>
        struct operation
        {
            constexpr decltype(auto) start() noexcept
            {
                resolver.async_resolve(host, service, [receiver = std::move(receiver)](error_code_t ec, results_type results) mutable
                {
                    if (!ec)
                        unifex::set_value(std::move(receiver), results);
                    else
                        unifex::set_error(std::move(receiver), ec);
                });
            }

            Receiver receiver;
            tcp::resolver resolver;

            std::string host;
            std::string service;
        };

        template <typename Receiver>
        constexpr decltype(auto) connect(Receiver&& receiver)
        {
            return operation<std::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver), tcp::resolver{context}, host, service};
        }

        Context& context;

        std::string host;
        std::string service;
    };
}

#endif
