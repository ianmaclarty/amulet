set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%
make TARGET=msvc64.release LUAVM=luajit
make TARGET=msvc64.release LUAVM=lua51
make TARGET=msvc64.release LUAVM=lua52