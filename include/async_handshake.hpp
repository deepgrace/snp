//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#ifndef ASYNC_HANDSHAKE_HPP
#define ASYNC_HANDSHAKE_HPP

#include <boost/asio.hpp>
#include <unifex/receiver_concepts.hpp>

namespace snp
{
    namespace net = boost::asio;

    template <typename Stream, typename... Args>
    struct async_handshake
    {
        using error_code_t = boost::system::error_code;

        template <template <typename ...> typename Variant, template <typename ...> typename Tuple>
        using value_types = Variant<Tuple<>>;

        template <template <typename ...> typename Variant>
        using error_types = Variant<error_code_t>;

        static constexpr bool sends_done = true;

        async_handshake(Stream& stream, Args&&... args) : stream(stream), args_(std::forward<Args>(args)...)
        {
        }

        template <typename Receiver>
        struct operation
        {
            constexpr decltype(auto) start() noexcept
            {
                std::apply([this]<typename... Brgs>(Brgs&&... brgs)
                {
                    stream.async_handshake(std::forward<Brgs>(brgs)..., [receiver = std::move(receiver)](error_code_t ec) mutable
                    {
                        if (!ec)
                            unifex::set_value(std::move(receiver));
                        else
                            unifex::set_error(std::move(receiver), ec);
                    });
                }, std::move(args));
            }

            Receiver receiver;

            Stream& stream;
            std::tuple<Args...> args;
        };

        template <typename Receiver>
        constexpr decltype(auto) connect(Receiver&& receiver)
        {
            return operation<std::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver), stream, std::move(args_)};
        }

        Stream& stream;
        std::tuple<Args...> args_;
    };

    template <typename Stream, typename... Args>
    async_handshake(Stream& stream, Args&&... args) -> async_handshake<Stream, Args...>;
}

#endif
