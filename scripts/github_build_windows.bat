set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%
make TARGET=msvc64.release LUAVM=luajit test
make TARGET=msvc64.release LUAVM=lua51 test
make TARGET=msvc64.release LUAVM=lua52 test