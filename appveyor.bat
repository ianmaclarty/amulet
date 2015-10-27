call "%VS140COMNTOOLS%vsvars32.bat"
call "C:\emscripten\emsdk_env.bat"
SET PATH=C:\MinGW\bin;C:\MinGW\msys\1.0\bin;%PATH%
make TARGET=html.release
