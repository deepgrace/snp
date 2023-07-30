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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <snp.hpp>
#include <unifex/then.hpp>
#include <unifex/upon_error.hpp>

// g++ -std=c++23 -O3 -Os -I include -l uring example/stream_client.cpp -o /tmp/stream_client

using namespace unifex;
namespace net = boost::asio;

using net::local::stream_protocol;
using socket_t = stream_protocol::socket;

using endpoint_t = stream_protocol::endpoint;
using error_code_t = boost::system::error_code;

constexpr std::size_t length = 1024;

class client
{
public:
    client(net::io_context& ioc, const std::string& file) : socket(ioc), v{endpoint_t(file)}
    {
    }

    void run()
    {
        do_connect();
    }

    void do_connect()
    {
        snp::async_connect(socket, v)
        | unifex::then([this](endpoint_t ep)
          {
              on_connect();
          })
        | unifex::upon_error([]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_connect: " << error.message() << std::endl;
          })
        | snp::start_detached();
    }

    void on_connect()
    {
        do_write();
    }

    void do_write()
    {
        std::cout << "Enter message: ";

        std::cin.getline(request, length);
        size = std::strlen(request);

        snp::async_write(socket, net::buffer(request, size))
        | unifex::then([this](std::size_t bytes_transferred)
          {
              on_write();
          })
        | unifex::upon_error([]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_write: " << error.message() << std::endl;
          })
        | snp::start_detached();
    }

    void on_write()
    {
        do_read();
    }

    void do_read()
    {
        snp::async_read(socket, net::buffer(reply, size))
        | unifex::then([this](std::size_t bytes_transferred)
          {
              on_read(bytes_transferred);
          })
        | unifex::upon_error([]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_read: " << error.message() << std::endl;
          })
        | snp::start_detached();
    }

    void on_read(std::size_t bytes_transferred)
    {
        std::cout << "Reply is: ";
        std::cout.write(reply, bytes_transferred);

        std::cout << std::endl;
    }

private:
    char request[length];
    char reply[length];

    socket_t socket; 

    std::size_t size;
    std::vector<endpoint_t> v;
};


int main(int argc, char* argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
            std::cerr << "Example: " << argv[0] << " /tmp/sock" << std::endl;

            return 1;
        }

        net::io_context ioc;
        client c(ioc, argv[1]);

        c.run();
        ioc.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
