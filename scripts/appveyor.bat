call "%VS140COMNTOOLS%vsvars32.bat"
set PATH=C:\emscripten;C:\MinGW\bin;C:\MinGW\msys\1.0\bin;%PATH%

if "%APPVEYOR_REPO_TAG_NAME:~-15%" == "-distro-trigger" goto builddistro

REM call "C:\emscripten\emsdk_env.bat"
REM make TARGET=html.release
REM if %errorlevel% neq 0 exit /b %errorlevel%
make TARGET=msvc32.release
if %errorlevel% neq 0 exit /b %errorlevel%
if defined APPVEYOR_REPO_TAG_NAME (node scripts\upload_builds.js %APPVEYOR_REPO_TAG_NAME%)
if %errorlevel% neq 0 exit /b %errorlevel%
goto end

:builddistro
set TAG=%APPVEYOR_REPO_TAG_NAME:~0,-15%
choco install -y InnoSetup
set PATH=%PATH%;"C:\Program Files (x86)\Inno Setup 5"
mingw-get install zip
echo step1
curl -L -O https://github.com/ianmaclarty/amulet/releases/download/%TAG%/builds-darwin.zip
curl -L -O https://github.com/ianmaclarty/amulet/releases/download/%TAG%/builds-win32.zip
curl -L -O https://github.com/ianmaclarty/amulet/releases/download/%TAG%/builds-linux.zip
echo step1b
unzip builds-darwin.zip
unzip builds-win32.zip
unzip builds-linux.zip
echo step2
mkdir scripts\installer-payload
echo step3
cp -r builds scripts/installer-payload/
echo step4
cp -r builds/msvc/lua51/release/bin/* scripts/installer-payload/
echo step5
cp scripts/icon.ico scripts/installer-payload/
echo step6
iscc /DVERSION=%TAG% scripts\installer.iss
if %errorlevel% neq 0 exit /b %errorlevel%
echo step7
cp scripts/output/setup.exe amulet-%TAG%-windows.exe
echo step8
mv scripts/installer-payload amulet-%TAG%
echo step9
zip -r amulet-%TAG%-windows.zip amulet-%TAG%
echo step10
node scripts\upload_distros.js %TAG% amulet-%TAG%-windows.exe amulet-%TAG%-windows.zip
if %errorlevel% neq 0 exit /b %errorlevel%
echo step11

:end
