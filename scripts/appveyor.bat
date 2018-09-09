call "%programfiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;"C:\Program Files (x86)\Inno Setup 5";%PATH%
zip -r test.zip Makefile
find /c/msys64 -type f -name "zip.exe"
find /c/msys64 -type f -name "zip.*"
find /c/msys64 -type f -name "unzip.*"
which unzip
find /c -type f -name "zip.exe"
find /c -type f -name "unzip.exe"
echo DONE
REM 
REM if "%APPVEYOR_REPO_TAG_NAME:~-15%" == "-distro-trigger" goto builddistro
REM 
REM make TARGET=msvc32.release LUAVM=lua51 test
REM make TARGET=msvc32.release LUAVM=lua52 test
REM make TARGET=msvc32.release LUAVM=luajit test
REM if %errorlevel% neq 0 exit /b %errorlevel%
REM if defined APPVEYOR_REPO_TAG_NAME (node scripts\upload_builds.js %APPVEYOR_REPO_TAG_NAME%)
REM if %errorlevel% neq 0 exit /b %errorlevel%
REM goto end
REM 
REM :builddistro
REM set TAG=%APPVEYOR_REPO_TAG_NAME:~0,-15%
REM choco install -y InnoSetup
REM curl -L -O https://github.com/ianmaclarty/amulet/releases/download/%TAG%/builds-darwin.zip
REM curl -L -O https://github.com/ianmaclarty/amulet/releases/download/%TAG%/builds-win32.zip
REM curl -L -O https://github.com/ianmaclarty/amulet/releases/download/%TAG%/builds-linux.zip
REM unzip builds-darwin.zip
REM unzip builds-win32.zip
REM unzip builds-linux.zip
REM mkdir scripts\installer-payload
REM cp -r builds scripts/installer-payload/
REM cp -r builds/msvc32/luajit/release/bin/* scripts/installer-payload/
REM mv scripts/installer-payload/amulet.exe scripts/installer-payload/amulet-window.exe
REM mv scripts/installer-payload/amulet-console.exe scripts/installer-payload/amulet.exe
REM cp -r examples scripts/installer-payload/
REM cp scripts/icon.ico scripts/installer-payload/
REM cp scripts/amulet_env.bat scripts/installer-payload/
REM iscc /DVERSION=%TAG% /O. /Famulet-%TAG%-windows scripts\installer.iss
REM if %errorlevel% neq 0 exit /b %errorlevel%
REM mv scripts/installer-payload amulet-%TAG%
REM zip -r amulet-%TAG%-windows.zip amulet-%TAG%
REM node scripts\upload_distros.js %TAG% amulet-%TAG%-windows.exe amulet-%TAG%-windows.zip
REM if %errorlevel% neq 0 exit /b %errorlevel%
REM 
REM :end
