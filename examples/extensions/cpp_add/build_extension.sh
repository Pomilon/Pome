#!/bin/bash
mkdir -p build
cd build
cmake ..
make
cp libmy_extension.so ../../../
echo "Extension built and copied to project root."
