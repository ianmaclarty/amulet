pacman -S --noconfirm zip

if "%APPVEYOR_REPO_TAG_NAME:~-15%" == "-distro-trigger" goto builddistro

call "%programfiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%
make STRICT=1 TARGET=msvc64.release LUAVM=luajit test
make STRICT=1 TARGET=msvc64.release LUAVM=lua51 test
make STRICT=1 TARGET=msvc64.release LUAVM=lua52 test
call "%programfiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
make STRICT=1 TARGET=msvc32.release LUAVM=luajit test
make STRICT=1 TARGET=msvc32.release LUAVM=lua51 test
make STRICT=1 TARGET=msvc32.release LUAVM=lua52 test
if %errorlevel% neq 0 exit /b %errorlevel%
if defined APPVEYOR_REPO_TAG_NAME (node scripts\upload_builds.js %APPVEYOR_REPO_TAG_NAME%)
if %errorlevel% neq 0 exit /b %errorlevel%
goto end

:builddistro
call "%programfiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;"C:\Program Files (x86)\Inno Setup 5";%PATH%
set TAG=%APPVEYOR_REPO_TAG_NAME:~0,-15%
choco install -y InnoSetup

rem appveyor has no zip command, so install one
curl -L -O https://github.com/bmatzelle/gow/archive/master.zip
unzip master.zip
set PATH=%PATH%;gow-master\bin\

curl -L -O https://github.com/ianmaclarty/amulet/releases/download/%TAG%/builds-darwin.zip
curl -L -O https://github.com/ianmaclarty/amulet/releases/download/%TAG%/builds-win32.zip
curl -L -O https://github.com/ianmaclarty/amulet/releases/download/%TAG%/builds-linux.zip
unzip builds-darwin.zip
unzip builds-win32.zip
unzip builds-linux.zip
mkdir scripts\installer-payload
cp -r templates scripts/installer-payload/
cp -r builds scripts/installer-payload/
cp -r builds/msvc32/luajit/release/bin/* scripts/installer-payload/
mv scripts/installer-payload/amulet.exe scripts/installer-payload/amulet-window.exe
mv scripts/installer-payload/amulet-console.exe scripts/installer-payload/amulet.exe
cp -r examples scripts/installer-payload/
cp scripts/icon.ico scripts/installer-payload/
cp scripts/amulet_env.bat scripts/installer-payload/
iscc /DVERSION=%TAG% /O. /Famulet-%TAG%-windows scripts\installer.iss
if %errorlevel% neq 0 exit /b %errorlevel%
mv scripts/installer-payload amulet-%TAG%
zip -r amulet-%TAG%-windows.zip amulet-%TAG%
node scripts\upload_distros.js %TAG% amulet-%TAG%-windows.exe amulet-%TAG%-windows.zip
if %errorlevel% neq 0 exit /b %errorlevel%

:end
