#!/bin/sh
set -e
if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    make TARGET=linux64.release LUAVM=lua51   test
    make TARGET=linux64.release LUAVM=lua52   test
    make TARGET=linux64.release LUAVM=luajit  test
    make TARGET=mingw32.release
else
    make TARGET=osx.release LUAVM=lua51   test
    make TARGET=osx.release LUAVM=luajit  test
    make TARGET=ios32.release LUAVM=lua51
    make TARGET=ios64.release LUAVM=lua51
fi
