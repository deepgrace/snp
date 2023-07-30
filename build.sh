#!/bin/bash

dst=/tmp
path=example

flags=(-std=c++23 -Wall -O3 -Os -I ../include -l uring)

cd ${path}

for file in *.cpp; do
    bin=${dst}/${file/.cpp/}

    g++ "${flags[@]}" -o ${bin} ${file}
    strip ${bin}
done

echo Please check the executables at ${dst}
