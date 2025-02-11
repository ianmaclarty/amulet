#!/bin/sh
set -e

# build osx
make TARGET=osx.release     LUAVM=lua51   test
make TARGET=osx.release     LUAVM=lua52   test
make TARGET=osx.release     LUAVM=luajit  test

# build ios
make TARGET=ios.release     LUAVM=lua51
make TARGET=ios.release     LUAVM=lua52