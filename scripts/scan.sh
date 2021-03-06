#! /bin/bash
# MacOS and homebrew specific
cd ..
rm -rf build/
mkdir build
cd build || exit
cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja -DCMAKE_C_COMPILER="$(locate -l 1 ccc-analyzer)" -DCMAKE_CXX_COMPILER="$(locate -l 1 c++-analyzer)" -DCMAKE_TOOLCHAIN_FILE="$HOME"/vcpkg/scripts/buildsystems/vcpkg.cmake ..
$(locate -l 1 scan-build) -o "$(pwd)"/scanresults -v ninja
