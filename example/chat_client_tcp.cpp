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
#include <snp.hpp>
#include <unifex/on.hpp>
#include <unifex/then.hpp>
#include <unifex/just_from.hpp>
#include <unifex/upon_error.hpp>
#include "chat_message.hpp"

// g++ -std=c++23 -Wall -O3 -Os -s -I include -l uring example/chat_client_tcp.cpp -o /tmp/chat_client_tcp

namespace net = boost::asio;

using tcp = net::ip::tcp;
using socket_t = tcp::socket;

using endpoint_t = socket_t::endpoint_type;
using error_code_t = boost::system::error_code;

using results_type = tcp::resolver::results_type;
using chat_message_queue = std::deque<chat_message>;

class chat_client
{
public:
    chat_client(snp::asio_context& ctx, const std::string& host, const std::string& port) :
    ctx(ctx), ioc(ctx.get_io_context()), sch(ctx.get_scheduler()), socket(ioc), host(host), port(port)
    {
        do_resolve();
    }

    void write(const chat_message& msg)
    {
        unifex::on(sch, unifex::just_from([this, msg]
        {
            bool write_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(msg);

            if (!write_in_progress)
                do_write();
        }))
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, std::exception_ptr>)
              {
                  try
                  {
                      if (error)
                          std::rethrow_exception(error);
                  }
                  catch (const std::exception& e)
                  {
                      std::cout << "caught exception: '" << e.what() << "'" << std::endl;
                  }
              }
          })
        | snp::start_detached();
    }

    void close()
    {
        snp::async_close(socket, ioc)
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
        | unifex::then([this](results_type endpoints)
          {
              on_resolve(endpoints);
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_resolve: " << error.message() << std::endl;

              socket.close();
          })
        | snp::start_detached();
    }

    void on_resolve(const results_type& endpoints)
    {
        do_connect(endpoints);
    }

    void do_connect(const results_type& endpoints)
    {
        snp::async_connect(socket, endpoints)
        | unifex::then([this](endpoint_t e)
          {
              on_connect();
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_connect: " << error.message() << std::endl;

              socket.close();
          })
        | snp::start_detached();
    }

    void on_connect()
    {
        do_read_header();
    }

    void do_read_header()
    {
        snp::async_read(socket, net::buffer(read_msg_.data(), chat_message::header_length))
        | unifex::then([this](std::size_t bytes_transferred)
          {
              on_read_header();
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_read: " << error.message() << std::endl;

              socket.close();
          })
        | snp::start_detached();
    }

    void on_read_header()
    {
        if (read_msg_.decode_header())
            do_read_body();
        else
            socket.close();
    }

    void do_read_body()
    {
        snp::async_read(socket, net::buffer(read_msg_.body(), read_msg_.body_length()))
        | unifex::then([this](std::size_t bytes_transferred)
          {
              on_read_body();
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_read: " << error.message() << std::endl;

              socket.close();
          })
        | snp::start_detached();
    }

    void on_read_body()
    {
        std::cout.write(read_msg_.body(), read_msg_.body_length());
        std::cout << std::endl;

        do_read_header();
    }

    void do_write()
    {
        auto& msg = write_msgs_.front();

        snp::async_write(socket, net::buffer(msg.data(), msg.length()))
        | unifex::then([&](std::size_t bytes_transferred) 
          {
              on_write();
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_write: " << error.message() << std::endl;

              socket.close();
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
    snp::asio_context& ctx;
    net::io_context& ioc;

    snp::asio_scheduler sch;
    socket_t socket;

    std::string host;
    std::string port;

    chat_message read_msg_;
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

        snp::asio_context ctx;
        chat_client c(ctx, argv[1], argv[2]);

        std::thread t([&ctx]{ ctx.run(); });
        char line[chat_message::max_body_length + 1];

        while (std::cin.getline(line, chat_message::max_body_length + 1))
        {
            chat_message msg;
            msg.body_length(std::strlen(line));

            std::memcpy(msg.body(), line, msg.body_length());
            msg.encode_header();

            c.write(msg);
        }

        c.close();
        t.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
