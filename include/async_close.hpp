//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#ifndef ASYNC_CLOSE_HPP
#define ASYNC_CLOSE_HPP

#include <boost/asio.hpp>
#include <unifex/receiver_concepts.hpp>

namespace snp
{
    namespace net = boost::asio;

    template <typename Stream, typename Context>
    struct async_close
    {
        using error_code_t = boost::system::error_code;

        template <template <typename ...> typename Variant, template <typename ...> typename Tuple>
        using value_types = Variant<Tuple<>>;

        template <template <typename ...> typename Variant>
        using error_types = Variant<error_code_t>;

        static constexpr bool sends_done = true;

        async_close(Stream& stream, Context&& context) : stream(stream), context(std::forward<Context>(context))
        {
        }

        template <typename Receiver>
        struct operation
        {
            constexpr decltype(auto) start() noexcept
            {
                if constexpr(requires { typename Stream::is_deflate_supported; })
                    stream.async_close(context, [receiver = std::move(receiver)](error_code_t ec) mutable
                    {
                        if (!ec)
                            unifex::set_value(std::move(receiver));
                        else
                            unifex::set_error(std::move(receiver), ec);
                    });
                else
                    net::post(context, [&]
                    {
                        error_code_t ec;
                        stream.close(ec);

                        if (!ec)
                            unifex::set_value(std::move(receiver));
                        else
                            unifex::set_error(std::move(receiver), ec);
                    });
            }

            Receiver receiver;

            Stream& stream;
            Context context;
        };

        template <typename Receiver>
        constexpr decltype(auto) connect(Receiver&& receiver)
        {
            return operation<std::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver), stream, context};
        }

        Stream& stream;
        Context context;
    };

    template <typename Stream, typename Context>
    async_close(Stream& stream, Context&& context) -> async_close<Stream, Context>;
}

#endif
