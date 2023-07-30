#
# Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Official repository: https://github.com/deepgrace/snp
#

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_FLAGS "-std=c++23 -Wall -O3 -Os -s")

include(GNUInstallDirs)
include_directories(${PROJECT_SOURCE_DIR}/include)

function(add_file NAME)
    add_executable("${NAME}" "${NAME}.cpp")
    target_link_libraries("${NAME}" uring)
    install(TARGETS ${NAME} DESTINATION ${PROJECT_SOURCE_DIR}/bin)
endfunction()

file(GLOB SRCS "*.cpp")

foreach(file-path ${SRCS})
    string(REPLACE ".cpp" "" file-path-without-ext ${file-path})
    get_filename_component(file-name ${file-path-without-ext} NAME)
    add_file(${file-name})
endforeach()
