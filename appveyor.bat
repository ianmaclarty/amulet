DIR C:\emscripten\bin
DIR C:\emscripten\emscripten\1.34.1
call "%VS140COMNTOOLS%vsvars32.bat"
SET PATH=C:\emscripten;C:\emscripten\bin;C:\MinGW\bin;C:\MinGW\msys\1.0\bin;%PATH%
call "C:\emscripten\emsdk_env.bat"
make TARGET=html.release
if %errorlevel% neq 0 exit /b %errorlevel%
make TARGET=msvc32.release
if %errorlevel% neq 0 exit /b %errorlevel%
