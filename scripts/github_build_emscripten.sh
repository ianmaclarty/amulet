#!/bin/sh
set -e

EMSDK=sdk-1.38.48-64bit
git clone https://github.com/juj/emsdk.git emscripten
cd emscripten
./emsdk install $EMSDK
./emsdk activate $EMSDK
source ./emsdk_env.sh
cd ..
make TARGET=html.release LUAVM=lua51