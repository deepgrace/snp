# snp [![LICENSE](https://img.shields.io/github/license/deepgrace/snp.svg)](https://github.com/deepgrace/snp/blob/master/LICENSE_1_0.txt) [![Language](https://img.shields.io/badge/language-C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support) [![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20MacOS%20%7C%20Windows-lightgrey.svg)](https://github.com/deepgrace/snp)

> **Structured Network Programming**

## Overview
```cpp
#define BOOST_ASIO_HAS_IO_URING
#define BOOST_ASIO_DISABLE_EPOLL

#include <iostream>
#include <snp.hpp>
#include <unifex/just.hpp>
#include <unifex/then.hpp>
#include <unifex/let_value.hpp>
#include <unifex/upon_error.hpp>

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
    | unifex::then([&](results_type endpoints)
      {
          return snp::async_connect(socket, endpoints);
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
```

## Introduction
snp is a network programming library, which is header-only, extensible and modern C++ oriented.  
It's built on top off the unifex and boost, it's inspired by the C++ *Sender / Receiver* async programming model.  
snp enables you to do network programming with tcp and websocket protocols in an asynchronous and structured manner.  

snp provides the following sender factories:
- **async_accept**
- **async_close**
- **async_connect**
- **async_handshake**
- **async_read**
- **async_read_some**
- **async_read_some_at**
- **async_resolve**
- **async_wait**
- **async_wait_until**
- **async_write**
- **async_write_some**
- **async_write_some_at**

snp provides the following sender consumer:
- **start_detached**

snp provides the following scheduler type:
- **asio_context**

snp provides the following scheduler algorithms:
- **now**
- **schedule**
- **schedule_at**
- **schedule_after**

## Prerequsites
[boost](https://www.boost.org)  
[uring](https://github.com/axboe/liburing)  
[unifex](https://github.com/facebookexperimental/libunifex)  

By default, the example is using the io_uring backend, it's disabled by default in Boost.Asio.  
This backend may be used for all I/O objects, including sockets, timers, and posix descriptors.  
To disable the io_uring backend, just comment out the following lines in code under the example directory:
```cpp
#define BOOST_ASIO_HAS_IO_URING
#define BOOST_ASIO_DISABLE_EPOLL
```

## Compiler requirements
The library relies on a C++20 compiler and standard library

More specifically, snp requires a compiler/standard library supporting the following C++20 features (non-exhaustively):
- concepts
- lambda templates
- All the C++20 type traits from the <type_traits> header

## Building
snp is header-only. To use it just add the necessary `#include` line to your source files, like this:
```cpp
#include <snp.hpp>
```

To build the example with cmake, `cd` to the root of the project and setup the build directory:
```bash
mkdir build
cd build
cmake ..
```

Make and install the executables:
```
make -j4
make install
```

The executables are now located at the `bin` directory of the root of the project.  
The example can also be built with the script `build.sh`, just run it, the executables will be put at the `/tmp` directory.

## Full example
Please see [example](example).

## License
snp is licensed as [Boost Software License 1.0](LICENSE_1_0.txt).
