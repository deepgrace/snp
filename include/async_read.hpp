//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#ifndef ASYNC_READ_HPP
#define ASYNC_READ_HPP

#include <boost/asio.hpp>
#include <unifex/receiver_concepts.hpp>

namespace snp
{
    namespace net = boost::asio;

    template <typename Stream, typename Buffer>
    struct async_read
    {
        using error_code_t = boost::system::error_code;

        template <typename T, typename U, typename V = net::mutable_buffer>
        using buffer_t = std::conditional_t<requires { typename T::is_deflate_supported; }, U, V>;

        template <template <typename ...> typename Variant, template <typename ...> typename Tuple>
        using value_types = Variant<Tuple<std::size_t>>;

        template <template <typename ...> typename Variant>
        using error_types = Variant<error_code_t>;

        static constexpr bool sends_done = true;

        async_read(Stream& stream, Buffer&& buffer) : stream(stream), buffer(std::forward<Buffer>(buffer))
        {
        }

        template <typename Receiver>
        struct operation
        {
            constexpr decltype(auto) start() noexcept
            {
                auto cb = [receiver = std::move(receiver)](error_code_t ec, std::size_t bytes_transferred) mutable
                {
                    if (!ec)
                        unifex::set_value(std::move(receiver), bytes_transferred);
                    else
                        unifex::set_error(std::move(receiver), ec);
                };

                if constexpr(requires { typename Stream::is_deflate_supported; })
                    stream.async_read(buffer, cb);
                else
                    net::async_read(stream, buffer, cb);
            }

            Receiver receiver;

            Stream& stream;
            buffer_t<Stream, Buffer> buffer;
        };

        template <typename Receiver>
        constexpr decltype(auto) connect(Receiver&& receiver)
        {
            return operation<std::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver), stream, buffer};
        }

        Stream& stream;
        buffer_t<Stream, Buffer> buffer;
    };

    template <typename Stream, typename Buffer>
    async_read(Stream& stream, Buffer&& buffer) -> async_read<Stream, Buffer>;
}

#endif
