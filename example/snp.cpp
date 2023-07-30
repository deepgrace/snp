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
#include <unifex/then.hpp>
#include <unifex/upon_error.hpp>

// g++ -std=c++23 -O3 -Os -I include -l uring example/snp.cpp -o /tmp/snp

namespace net = boost::asio;

using tcp = net::ip::tcp;
using socket_t = tcp::socket;

using error_code_t = boost::system::error_code;
using results_type = tcp::resolver::results_type;

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <text>" << std::endl;

        return 1;
    }

    std::string buffer;
    buffer.resize(100);

    net::io_context ioc;
    socket_t socket(ioc); 

    std::string host = argv[1];
    std::string port = argv[2];

    std::string text = argv[3];

    snp::async_resolve(ioc, host, port) 
    | unifex::then([&](results_type results)
      {
          for (auto& r : results)
          {
               auto ep = r.endpoint();

               std::cout << "address   " << ep.address()  << " port         " << ep.port()        << std::endl;
               std::cout << "host_name " << r.host_name() << " service_name " << r.service_name() << std::endl;
          }

          snp::async_connect(socket, results)
          | unifex::then([&](tcp::endpoint e)
            {
                std::cout << "connected to " << e.address() << " " << e.port() << std::endl;

                snp::async_write(socket, net::buffer(text))
                | unifex::then([&](std::size_t bytes_transferred)
                  {
                      std::cout << "transferred " << bytes_transferred << " bytes" << std::endl;

                      snp::async_read_some(socket, net::buffer(buffer))
                      | unifex::then([&](std::size_t bytes_received)
                        {
                            std::cout << "bytes_received \e[38;5;42;1m" << buffer << "\e[0m" << std::endl;
                        })
                      | unifex::upon_error([]<typename Error>(Error error)
                        {
                            if constexpr(std::is_same_v<Error, error_code_t>)
                                std::cout << "async_read_some: " << error.message() << std::endl;
                        })
                      | snp::start_detached();

                  })
                | unifex::upon_error([]<typename Error>(Error error)
                  {
                      if constexpr(std::is_same_v<Error, error_code_t>)
                          std::cout << "async_write: " << error.message() << std::endl;
                  })
                | snp::start_detached();
            })
          | unifex::upon_error([]<typename Error>(Error error)
            {
                if constexpr(std::is_same_v<Error, error_code_t>)
                    std::cout << "async_connect: " << error.message() << std::endl;
            })
          | snp::start_detached();

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
