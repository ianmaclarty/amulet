#!/bin/bash
set -e
if [[ "$TRAVIS_TAG" == *-distro-trigger ]]; then
    TAG=${TRAVIS_TAG%-distro-trigger}
    VERSION=`echo $TAG | tr -d v`
    curl -L -O https://github.com/ianmaclarty/amulet/releases/download/${TAG}/builds-darwin.zip
    curl -L -O https://github.com/ianmaclarty/amulet/releases/download/${TAG}/builds-win32.zip
    curl -L -O https://github.com/ianmaclarty/amulet/releases/download/${TAG}/builds-linux.zip
    unzip builds-darwin.zip
    unzip builds-win32.zip
    unzip builds-linux.zip
    mkdir amulet-${TAG}
    if [ "$TRAVIS_OS_NAME" = "linux" ]; then
        cp -r builds/linux64/luajit/release/bin/* amulet-${TAG}/
        cp -r builds/linux32/luajit/release/bin/amulet amulet-${TAG}/amulet.i686
        chmod a+x amulet-${TAG}/amulet
        chmod a+x amulet-${TAG}/amulet.i686
    else
        cp -r builds/osx/luajit/release/bin/* amulet-${TAG}/
        chmod a+x amulet-${TAG}/amulet
    fi
    mv builds amulet-${TAG}/
    zip -r amulet-${TAG}-${TRAVIS_OS_NAME}.zip amulet-${TAG}
    FILE_LIST=amulet-${TAG}-${TRAVIS_OS_NAME}.zip
    if [ "$TRAVIS_OS_NAME" = "osx" ]; then
        mkdir -p pkg_payload/usr/local/bin
        mkdir -p pkg_payload/usr/local/share/amulet
        mv amulet-${TAG}/builds pkg_payload/usr/local/share/amulet/
        mv amulet-${TAG}/amulet pkg_payload/usr/local/bin/
        pkgbuild --identifier xyz.amulet.pkg --version $VERSION --root pkg_payload/ --install-location / amulet-${TAG}-${TRAVIS_OS_NAME}.pkg
        FILE_LIST="$FILE_LIST amulet-${TAG}-${TRAVIS_OS_NAME}.pkg"
    fi
    scripts/upload_distros.js $TAG $FILE_LIST
else
    if [ "$TRAVIS_OS_NAME" = "linux" ]; then
        # build linux
        make -j2 TARGET=linux32.release LUAVM=lua51   test
        #make -j2 TARGET=linux32.release LUAVM=lua52   test
        #make -j2 TARGET=linux32.release LUAVM=luajit  test
        #make -j2 TARGET=linux64.release LUAVM=lua51   test
        #make -j2 TARGET=linux64.release LUAVM=lua52   test
        #make -j2 TARGET=linux64.release LUAVM=luajit  test
        #make -j2 TARGET=mingw32.release LUAVM=lua51
        #make -j2 TARGET=mingw32.release LUAVM=lua52
        #scripts/gen_linux_universal.sh

        # build android
        #NDK=android-ndk-r14b
        #echo downloading $NDK...
        #curl -s -L https://dl.google.com/android/repository/$NDK-linux-x86_64.zip -o android-ndk.zip
        #echo unzipping $NDK...
        #unzip android-ndk.zip > /dev/null
        #export NDK_HOME=`pwd`/$NDK
        #export NDK_HOST=linux-x86_64
        #export NDK_ANDROID_VER=16
        #make -j2 TARGET=android.release LUAVM=lua51
        #make -j2 TARGET=android.release LUAVM=lua52

    else
        # build emscripten
        EMSDK=sdk-1.38.8-64bit
        git clone https://github.com/juj/emsdk.git emscripten
        cd emscripten
        ./emsdk install --enable-wasm $EMSDK
        ./emsdk activate $EMSDK
        source ./emsdk_env.sh
        cd ..
        make -j2 TARGET=html.release LUAVM=lua51

        #make -j2 TARGET=osx.release     LUAVM=lua51   test
        #make -j2 TARGET=osx.release     LUAVM=lua52   test
        #make -j2 TARGET=osx.release     LUAVM=luajit  test
        #scripts/build_ios.sh lua51
        #rm -rf builds/ios32/lua51
        #rm -rf builds/ios64/lua51
        #scripts/build_ios.sh lua52
        #rm -rf builds/ios32/lua52
        #rm -rf builds/ios64/lua52
    fi
    if [ -n "$TRAVIS_TAG" ]; then
        scripts/upload_builds.js $TRAVIS_TAG
    fi
fi
scripts/update_site.sh
