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

#include <deque>
#include <thread>
#include <iostream>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <snp.hpp>
#include <unifex/then.hpp>
#include <unifex/upon_error.hpp>

// g++ -std=c++23 -O3 -Os -I include -l uring example/chat_client_ws.cpp -o /tmp/chat_client_ws

namespace net = boost::asio;
namespace beast = boost::beast;

namespace http = beast::http;
namespace websocket = beast::websocket;

template <typename T>
using websocket_t = websocket::stream<T>;

using tcp = net::ip::tcp;
using buffer_t = beast::flat_buffer;

using error_code_t = boost::system::error_code;
using socket_t = websocket_t<beast::tcp_stream>;

using results_type = tcp::resolver::results_type;
using endpoint_t = results_type::endpoint_type;

using chat_message_queue = std::deque<std::string>;

class chat_client
{
public:
    chat_client(net::io_context& ioc, const std::string& host, const std::string& port) : ioc(ioc), socket(net::make_strand(ioc)), host(host), port(port)
    {
        do_resolve();
    }

    void write(const std::string& msg)
    {
        net::post(ioc, [this, msg]
        {
            bool write_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(msg);

            if (!write_in_progress)
                do_write();
        });
    }

    void close()
    {
        snp::async_close(socket, websocket::close_code::normal)
        | unifex::then([this]
          {
              on_close();
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_close: " << error.message() << std::endl;
          })
        | snp::start_detached();
    }

    void on_close()
    {
    }

private:
    void do_resolve()
    {
        snp::async_resolve(ioc, host, port)
        | unifex::then([this](results_type results) 
          {
              on_resolve(results);
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_resolve: " << error.message() << std::endl;

              close();
          })
        | snp::start_detached();
    }

    void on_resolve(const results_type& results)
    {
        do_connect(results);
    }

    void do_connect(const results_type& results)
    {
        auto& layer = beast::get_lowest_layer(socket);
        layer.expires_after(std::chrono::seconds(30));

        snp::async_connect(layer.socket(), results)
        | unifex::then([this](endpoint_t e)
          {
              on_connect(e);
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_connect: " << error.message() << std::endl;

              close();
          })
        | snp::start_detached();
    }

    void on_connect(const endpoint_t& e)
    {
        do_handshake(e);
    }

    void do_handshake(const endpoint_t& e)
    {
        beast::get_lowest_layer(socket).expires_never();
        socket.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

        socket.set_option(websocket::stream_base::decorator([](websocket::request_type& req)
        {
            req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");
        }));

        host += ':' + std::to_string(e.port());

        snp::async_handshake(socket, host, "/")
        | unifex::then([this]
          {
              on_handshake();
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_handshake: " << error.message() << std::endl;

              close();
          })
        | snp::start_detached();
    }

    void on_handshake()
    {
        do_read();
    }

    void do_read()
    {
        snp::async_read(socket, buffer)
        | unifex::then([this](std::size_t bytes_transferred)
          {
              on_read();
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_read: " << error.message() << std::endl;

              close();
          })
        | snp::start_detached();
    }

    void on_read()
    {
        std::cout << beast::make_printable(buffer.data()) << std::endl;
        buffer.consume(buffer.size());

        do_read();
    }

    void do_write()
    {
        auto& msg = write_msgs_.front();

        snp::async_write(socket, net::buffer(msg.data(), msg.size()))
        | unifex::then([&](std::size_t bytes_transferred) 
          {
              on_write();
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_write: " << error.message() << std::endl;

              close();
          })
        | snp::start_detached();
    }

    void on_write()
    {
        write_msgs_.pop_front();

        if (!write_msgs_.empty())
            do_write();
    }

private:
    net::io_context& ioc;
    socket_t socket;

    std::string host;
    std::string port;

    buffer_t buffer;
    chat_message_queue write_msgs_;
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl;

            return 1;
        }

        net::io_context ioc;
        chat_client c(ioc, argv[1], argv[2]);

        std::string line;
        std::thread t([&ioc]{ ioc.run(); });

        while (std::getline(std::cin, line))
               c.write(line);

        c.close();
        t.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
