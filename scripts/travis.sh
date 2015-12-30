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
        cp -r builds/linux64/lua51/release/bin/* amulet-${TAG}/
        cp -r builds/linux32/lua51/release/bin/amulet amulet-${TAG}/amulet.i686
    else
        cp -r builds/osx/lua51/release/bin/* amulet-${TAG}/
    fi
    mv builds amulet-${TAG}/
    zip -r amulet-${TAG}-${TRAVIS_OS_NAME}.zip amulet-${TAG}
    if [ "$TRAVIS_OS_NAME" = "osx" ]; then
        mkdir -p pkg_payload/usr/local/bin
        mkdir -p pkg_payload/usr/local/share/amulet
        mv amulet-${TAG}/builds pkg_payload/usr/local/share/amulet/
        mv amulet-${TAG}/amulet pkg_payload/usr/local/bin/
        pkgbuild --identifier xyz.amulet.pkg --version $VERSION --root pkg_payload/ --install-location / amulet-${TAG}-${TRAVIS_OS_NAME}.pkg
    fi
    scripts/upload_distros.js $TAG amulet-${TAG}-${TRAVIS_OS_NAME}.zip amulet-${TAG}-${TRAVIS_OS_NAME}.pkg
else
    if [ "$TRAVIS_OS_NAME" = "linux" ]; then
        make TARGET=linux64.release LUAVM=lua51   test
        make TARGET=linux64.release LUAVM=lua52   test
        make TARGET=linux64.release LUAVM=luajit  test
        make TARGET=mingw32.release
    else
        make TARGET=osx.release     LUAVM=lua51   test
        make TARGET=osx.release     LUAVM=luajit  test
        make TARGET=ios32.release   LUAVM=lua51
        make TARGET=ios64.release   LUAVM=lua51
    fi
    if [ -n "$TRAVIS_TAG" ]; then
        scripts/upload_builds.js $TRAVIS_TAG
    fi
fi
