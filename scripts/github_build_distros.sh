#!/bin/bash
set -e
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
    cp -r builds/osx/luajit/release/bin/amulet amulet-${TAG}/
    chmod a+x amulet-${TAG}/amulet
fi
mv templates amulet-${TAG}/
mv builds amulet-${TAG}/
zip -r amulet-${TAG}-${TRAVIS_OS_NAME}.zip amulet-${TAG}
FILE_LIST=amulet-${TAG}-${TRAVIS_OS_NAME}.zip
