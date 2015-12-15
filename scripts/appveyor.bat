if "%APPVEYOR_REPO_TAG_NAME:~-7%" neq "-staged" (
    call "%VS140COMNTOOLS%vsvars32.bat"
    SET PATH=C:\emscripten;C:\MinGW\bin;C:\MinGW\msys\1.0\bin;%PATH%
    call "C:\emscripten\emsdk_env.bat"
    make TARGET=html.release
    if %errorlevel% neq 0 exit /b %errorlevel%
    make TARGET=msvc32.release
    if %errorlevel% neq 0 exit /b %errorlevel%
    if defined APPVEYOR_REPO_TAG_NAME (node scripts\upload_release.js %APPVEYOR_REPO_TAG_NAME%)
    if %errorlevel% neq 0 exit /b %errorlevel%
) else (
    call scripts\build_windows_distro.bat
)
