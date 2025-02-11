pacman -S --noconfirm zip

call "%programfiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%
make STRICT=1 TARGET=msvc64.release LUAVM=luajit test
make STRICT=1 TARGET=msvc64.release LUAVM=lua51 test
make STRICT=1 TARGET=msvc64.release LUAVM=lua52 test