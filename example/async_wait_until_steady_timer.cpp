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

// g++ -std=c++23 -Wall -O3 -Os -s -I include -l uring example/async_wait_until_steady_timer.cpp -o /tmp/async_wait_until_steady_timer

namespace net = boost::asio;

using error_code_t = boost::system::error_code;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <number>" << std::endl;

        return 1;
    }

    net::io_context ioc;
    int number = std::stoi(argv[1]);

    bool timed_out = false;
    net::steady_timer timer(ioc);

    auto tp = std::chrono::steady_clock::now() + std::chrono::seconds(number);

    snp::async_wait_until(timer, tp)
    | unifex::then([&]
      {
          timed_out = true;
      })
    | unifex::upon_error([]<typename Error>(Error error)
      {
          if constexpr(std::is_same_v<Error, error_code_t>)
              std::cout << "async_wait_until steady_timer: " << error.message() << std::endl;
      })
    | snp::start_detached(); 

    assert(!timed_out);
    auto begin = std::chrono::steady_clock::now();

    ioc.run();

    auto end = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();

    std::cout << "async_wait_until steady_timer waited " << dur << " seconds" << std::endl;

    assert(timed_out);

    return 0;
}
