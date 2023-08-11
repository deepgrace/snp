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

#include <array>
#include <memory>
#include <iostream>
#include <snp.hpp>
#include <unifex/then.hpp>
#include <unifex/upon_error.hpp>

// g++ -std=c++23 -Wall -O3 -Os -s -I include -l uring example/stream_server.cpp -o /tmp/stream_server

using namespace unifex;
namespace net = boost::asio;

using net::local::stream_protocol;
using socket_t = stream_protocol::socket;

using endpoint_t = stream_protocol::endpoint;
using error_code_t = boost::system::error_code;

class session : public std::enable_shared_from_this<session>
{
public:
    session(socket_t socket) : socket(std::move(socket))
    {
    }

    void start()
    {
        do_read();
    }

    void do_read()
    {
        snp::async_read_some(socket, net::buffer(buff))
        | unifex::then([this, self = shared_from_this()](std::size_t bytes_transferred)
          {
              on_read(bytes_transferred);
          })
        | unifex::upon_error([]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
              {
                  if (error != net::error::eof)
                      std::cerr << "async_read_some: " << error.message() << std::endl;
              }
          })
        | snp::start_detached();
    }

    void on_read(std::size_t bytes_transferred)
    {
        do_write(bytes_transferred);
    }

    void do_write(std::size_t length)
    {
        snp::async_write(socket, net::buffer(buff, length))
        | unifex::then([this, self = shared_from_this()](std::size_t bytes_transferred)
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

private:
    socket_t socket;
    std::array<char, 1024> buff;
};

class server
{
public:
    server(net::io_context& ioc, const std::string& file) : acceptor(ioc, endpoint_t(file))
    {
        do_accept();
    }

    void do_accept()
    {
        snp::async_accept(acceptor)
        | unifex::then([this](socket_t socket)
          {
              std::make_shared<session>(std::move(socket))->start();
              do_accept();
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_accept: " << error.message() << std::endl;

              do_accept();
          })
        | snp::start_detached();
    }

private:
    stream_protocol::acceptor acceptor;
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
            std::cerr << "Example: " << argv[0] << " /tmp/sock" << std::endl;

            std::cerr << "*** WARNING: existing file is removed ***" << std::endl;

            return 1;
        }

        net::io_context ioc;

        std::remove(argv[1]);
        server s(ioc, argv[1]);

        ioc.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
