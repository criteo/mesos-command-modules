#!/bin/bash

set +e
set +x

cd mesos-command-modules

mkdir build
cd build

cmake ..

make clang-format

updated_files_count=`git diff --name-only | wc -l`

if [ "$updated_files_count" -ne "0" ];
then
  echo "[FAILED] Please run 'make clang-format' and commit the changes."
  exit 1
fi

make
make check
