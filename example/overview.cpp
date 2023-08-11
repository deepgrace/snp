//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/snp
//

#define BOOST_ASIO_HAS_IO_URING
#define BOOST_ASIO_DISABLE_EPOLL

#include <iostream>
#include <snp.hpp>
#include <unifex/just.hpp>
#include <unifex/then.hpp>
#include <unifex/let_value.hpp>
#include <unifex/upon_error.hpp>

// g++ -std=c++23 -Wall -O3 -Os -s -I include -l uring example/overview.cpp -o /tmp/overview

namespace net = boost::asio;

using tcp = net::ip::tcp;
using socket_t = tcp::socket;

using error_code_t = boost::system::error_code;
using results_type = tcp::resolver::results_type;

int main(int argc, char* argv[])
{
    net::io_context ioc;
    socket_t socket(ioc); 

    std::string port = "https";
    std::string host = "isocpp.org";

    snp::async_resolve(ioc, host, port) 
    | unifex::then([&](results_type results)
      {
          return snp::async_connect(socket, results);
      })
    | unifex::let_value([&]<typename T>(T&& t)
      {
          t | unifex::then([&](tcp::endpoint e)
            {
                std::cout << "connected to " << e.address() << " " << e.port() << std::endl;
            })
          | unifex::upon_error([]<typename Error>(Error error)
            {
                if constexpr(std::is_same_v<Error, error_code_t>)
                    std::cout << "async_connect: " << error.message() << std::endl;
            })
          | snp::start_detached();

          return unifex::just();
      })
    | unifex::upon_error([]<typename Error>(Error error)
      {
          if constexpr(std::is_same_v<Error, error_code_t>)
              std::cout << "async_resolve: " << error.message() << std::endl;
      })
    | snp::start_detached();

    ioc.run();

    return 0;
}
