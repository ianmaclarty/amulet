#!/bin/sh
set -e

# build osx
make TARGET=osx.release     LUAVM=lua51   test
make TARGET=osx.release     LUAVM=lua52   test
make TARGET=osx.release     LUAVM=luajit  test

# build ios
make TARGET=ios.release     LUAVM=lua51
make TARGET=ios.release     LUAVM=lua52

# build emscripten
# (building on osx, because the pre-built llvm binaries don't work on linux due to incompatible glibc version)
EMSDK=sdk-1.38.48-64bit
git clone https://github.com/juj/emsdk.git emscripten
cd emscripten
./emsdk install $EMSDK
./emsdk activate $EMSDK
source ./emsdk_env.sh
cd ..
make STRICT=1 TARGET=html.release LUAVM=lua51