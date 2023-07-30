//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#ifndef ASYNC_WRITE_SOME_HPP
#define ASYNC_WRITE_SOME_HPP

#include <boost/asio.hpp>
#include <unifex/receiver_concepts.hpp>

namespace snp
{
    namespace net = boost::asio;

    template <typename Stream>
    struct async_write_some
    {
        using error_code_t = boost::system::error_code;

        template <template <typename ...> typename Variant, template <typename ...> typename Tuple>
        using value_types = Variant<Tuple<std::size_t>>;

        template <template <typename ...> typename Variant>
        using error_types = Variant<error_code_t>;

        static constexpr bool sends_done =  true;

        template <typename Buffer>
        async_write_some(Stream& stream, Buffer&& buffer) : stream(stream), buffer(std::forward<Buffer>(buffer))
        {
        }

        template <typename Receiver>
        struct operation
        {
            constexpr decltype(auto) start() noexcept
            {
                stream.async_write_some(buffer, [receiver = std::move(receiver)](error_code_t ec, std::size_t bytes_transferred) mutable
                {
                    if (!ec)
                        unifex::set_value(std::move(receiver), bytes_transferred);
                    else
                        unifex::set_error(std::move(receiver), ec);
                });
            }

            Receiver receiver;

            Stream& stream;
            net::mutable_buffer buffer;
        };

        template <typename Receiver>
        constexpr decltype(auto) connect(Receiver&& receiver)
        {
            return operation<std::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver), stream, buffer};
        }

        Stream& stream;
        net::mutable_buffer buffer;
    };
}

#endif
