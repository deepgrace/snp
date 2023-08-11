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
#include <iostream>
#include <snp.hpp>
#include <unifex/then.hpp>
#include <unifex/upon_error.hpp>

// g++ -std=c++23 -Wall -O3 -Os -s -I include -l uring example/async_file_copy.cpp -o /tmp/async_file_copy

namespace net = boost::asio;

using file = net::stream_file;
using error_code_t = boost::system::error_code;

class file_copier
{
public:
    file_copier(net::io_context& ioc, const std::string& from, const std::string& to) :
    from(ioc, from, file::read_only), to(ioc, to, file::write_only | file::create | file::truncate)
    {
    }

    void start()
    {
        do_read();
    }

    void do_read()
    {
        snp::async_read_some(from, net::buffer(buff))
        | unifex::then([this](std::size_t bytes_transferred)
          {
              on_read(bytes_transferred);
          })
        | unifex::upon_error([]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
              {
                  if (error != net::error::eof)
                      std::cerr << "Error copying file: " << error.message() << std::endl;
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
        snp::async_write(to, net::buffer(buff, length))
        | unifex::then([this](std::size_t bytes_transferred)
          {
              on_write();
          })
        | unifex::upon_error([]<typename Error>(Error error)
          {
              if constexpr(std::is_same_v<Error, error_code_t>)
                  std::cerr << "Error copying file: " << error.message() << std::endl;
          })
        | snp::start_detached();
    }

    void on_write()
    {
        do_read();
    }

private:
    std::array<char, 4096> buff;

    file from;
    file to;
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " <from> <to>" << std::endl;

            return 1;
        }

        net::io_context ioc;

        file_copier copier(ioc, argv[1], argv[2]);
        copier.start();

        ioc.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
