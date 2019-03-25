#!/bin/bash

set +e
set +x

cd src/mesos-command-modules

mkdir build
cd build

cmake ..

export PATH=$PATH:/opt/llvm-3.8.0/bin

make
make check
