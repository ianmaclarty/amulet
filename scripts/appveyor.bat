call "%VS140COMNTOOLS%vsvars32.bat"
set PATH=C:\emscripten;C:\MinGW\bin;C:\MinGW\msys\1.0\bin;"C:\Program Files (x86)\Inno Setup 5";%PATH%

if "%APPVEYOR_REPO_TAG_NAME:~-15%" == "-distro-trigger" goto builddistro

curl -L -O https://s3.amazonaws.com/mozilla-games/emscripten/releases/emsdk-1.34.1-full-64bit.exe
emsdk-1.34.1-full-64bit.exe /S /D=C:\emscripten
call "C:\emscripten\emsdk_env.bat"
make TARGET=html.release
if %errorlevel% neq 0 exit /b %errorlevel%
make TARGET=msvc32.release
if %errorlevel% neq 0 exit /b %errorlevel%
if defined APPVEYOR_REPO_TAG_NAME (node scripts\upload_builds.js %APPVEYOR_REPO_TAG_NAME%)
if %errorlevel% neq 0 exit /b %errorlevel%
goto end

:builddistro
set TAG=%APPVEYOR_REPO_TAG_NAME:~0,-15%
choco install -y InnoSetup
curl -L -O https://github.com/ianmaclarty/amulet/releases/download/%TAG%/builds-darwin.zip
curl -L -O https://github.com/ianmaclarty/amulet/releases/download/%TAG%/builds-win32.zip
curl -L -O https://github.com/ianmaclarty/amulet/releases/download/%TAG%/builds-linux.zip
unzip builds-darwin.zip
unzip builds-win32.zip
unzip builds-linux.zip
mkdir scripts\installer-payload
cp -r builds scripts/installer-payload/
cp -r builds/msvc32/lua51/release/bin/* scripts/installer-payload/
cp -r examples scripts/installer-payload/
cp scripts/icon.ico scripts/installer-payload/
iscc /DVERSION=%TAG% scripts\installer.iss
if %errorlevel% neq 0 exit /b %errorlevel%
cp scripts/output/setup.exe amulet-%TAG%-windows.exe
mv scripts/installer-payload amulet-%TAG%
zip -r amulet-%TAG%-windows.zip amulet-%TAG%
node scripts\upload_distros.js %TAG% amulet-%TAG%-windows.exe amulet-%TAG%-windows.zip
if %errorlevel% neq 0 exit /b %errorlevel%

:end
