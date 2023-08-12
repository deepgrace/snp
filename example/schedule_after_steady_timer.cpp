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
#include <unifex/on.hpp>
#include <unifex/just.hpp>
#include <unifex/then.hpp>
#include <unifex/upon_error.hpp>
#include <unifex/scheduler_concepts.hpp>

// g++ -std=c++23 -Wall -O3 -Os -s -I include -l uring example/schedule_after_steady_timer.cpp -o /tmp/schedule_after_steady_timer

namespace net = boost::asio;

using error_code_t = boost::system::error_code;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <number>" << std::endl;

        return 1;
    }

    int number = std::stoi(argv[1]);

    bool timed_out = false;
    snp::asio_context ctx;

    auto sch = ctx.get_scheduler();
    auto duration = std::chrono::seconds(number);

    unifex::then(unifex::schedule_after(sch, duration), []{ return 16; })
    | unifex::then([&](int n)
      {
          timed_out = true;
          std::cout << "schedule_after steady_timer " << n << std::endl;
      })
    | unifex::upon_error([]<typename Error>(Error error)
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

    assert(!timed_out);
    auto begin = std::chrono::steady_clock::now();

    ctx.run();

    auto end = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();

    std::cout << "scheduler elapsed " << dur << " seconds" << std::endl;

    assert(timed_out);

    return 0;
}
