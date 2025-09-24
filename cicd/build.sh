#!/bin/bash

mkdir build
cmake -DCMAKE_PREFIX_PATH=/opt/Qt/6.8.3/gcc_64/ -S landrop-test -B build -G Ninja
cd build
ninja

