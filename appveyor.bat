SET PATH=C:\emscripten
call "C:\emscripten\emsdk_env.bat"
emmake TARGET=html.release
call "%VS140COMNTOOLS%vsvars32.bat"
SET PATH=C:\MinGW\bin;C:\MinGW\msys\1.0\bin;%PATH%
make
