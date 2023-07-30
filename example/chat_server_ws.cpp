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

#include <set>
#include <list>
#include <deque>
#include <iostream>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <snp.hpp>
#include <unifex/then.hpp>
#include <unifex/upon_error.hpp>

// g++ -std=c++23 -O3 -Os -I include -l uring example/chat_server_ws.cpp -o /tmp/chat_server_ws

using namespace unifex;

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

class chat_participant
{
public:
    virtual ~chat_participant(){}
    virtual void deliver(const std::string& msg) = 0;
};

using chat_participant_ptr = std::shared_ptr<chat_participant>;

class chat_room
{
public:
    void join(chat_participant_ptr participant)
    {
        participants_.insert(participant);

        for (auto& msg: recent_msgs_)
             participant->deliver(msg);
    }

    void leave(chat_participant_ptr participant)
    {
        participants_.erase(participant);
    }

    void deliver(const std::string& msg)
    {
        recent_msgs_.push_back(msg);

        while (recent_msgs_.size() > max_recent_msgs)
               recent_msgs_.pop_front();

        for (auto& participant: participants_)
             participant->deliver(msg);
    }

private:
    chat_message_queue recent_msgs_;
    static constexpr int max_recent_msgs = 100;

    std::set<chat_participant_ptr> participants_;
};

class chat_session : public chat_participant, public std::enable_shared_from_this<chat_session>
{
public:
    chat_session(tcp::socket socket, chat_room& room) : socket(std::move(socket)), room(room)
    {
    }

    void start()
    {
        net::dispatch(socket.get_executor(), [this, self = shared_from_this()]
        {
            on_start();
        });
    }

    void on_start()
    {
        socket.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

        socket.set_option(websocket::stream_base::decorator([](websocket::response_type& res)
        {
            res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async");
        }));

        snp::async_accept(socket)
        | unifex::then([this, self = shared_from_this()]
          {
              on_accept();
          })
        | unifex::upon_error([this, self = shared_from_this()]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_accept: " << error.message() << std::endl;
          })
        | snp::start_detached();
    }

    void on_accept()
    {
        room.join(shared_from_this());
        do_read();
    }

    void deliver(const std::string& msg)
    {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);

        if (!write_in_progress)
            do_write();
    }

private:
    void do_read()
    {
        snp::async_read(socket, buffer)
        | unifex::then([this, self = shared_from_this()](std::size_t bytes_transferred)
          {
              on_read();
          })
        | unifex::upon_error([this, self = shared_from_this()]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_read: " << error.message() << std::endl;

              room.leave(shared_from_this());
          })
        | snp::start_detached();
    }

    void on_read()
    {
        room.deliver(beast::buffers_to_string(buffer.data()));
        buffer.consume(buffer.size());

        do_read();
    }

    void do_write()
    {
        auto& msg = write_msgs_.front();

        snp::async_write(socket, net::buffer(msg.data(), msg.length()))
        | unifex::then([this, self = shared_from_this()](std::size_t bytes_transferred)
          {
              on_write();
          })
        | unifex::upon_error([this, self = shared_from_this()]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_write: " << error.message() << std::endl;

              room.leave(shared_from_this());
          })
        | snp::start_detached();
    }

    void on_write()
    {
        write_msgs_.pop_front();

        if (!write_msgs_.empty())
            do_write();
    }

    socket_t socket;
    chat_room& room;

    buffer_t buffer;
    chat_message_queue write_msgs_;
};

class chat_server
{
public:
    chat_server(net::io_context& ioc, const tcp::endpoint& endpoint) : acceptor(ioc, endpoint)
    {
        do_accept();
    }

private:
    void do_accept()
    {
        snp::async_accept(acceptor)
        | unifex::then([this](tcp::socket socket)
          {
              on_accept(std::move(socket));
          })
        | unifex::upon_error([this]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "async_accept: " << error.message() << std::endl;

              do_accept();
          })
        | snp::start_detached();
    }

    void on_accept(tcp::socket&& socket)
    {
        std::make_shared<chat_session>(std::move(socket), room)->start();
        do_accept();
    }

    chat_room room;
    tcp::acceptor acceptor;
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc < 2)
        {
            std::cerr << "Usage: " << argv[0] << " <port> [<port> ...]" << std::endl;

            return 1;
        }

        net::io_context ioc;
        std::list<chat_server> servers;

        for (int i = 1; i < argc; ++i)
        {
             tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
             servers.emplace_back(ioc, endpoint);
        }

        ioc.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
