#!/bin/bash

set +e
set +x

cd mesos-command-modules

mkdir build
cd build

cmake ..

export PATH=$PATH:/opt/llvm-3.8.0/bin
make clang-format

updated_files_count=`git diff --name-only | wc -l`

if [ "$updated_files_count" -ne "0" ];
then
  echo "[FAILED] Please run 'make clang-format' and commit the changes."
  exit 1
fi

make
make check
