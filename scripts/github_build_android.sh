#!/bin/sh
set -e

NDK=android-ndk-r20
echo downloading $NDK...
curl -s -L https://dl.google.com/android/repository/$NDK-linux-x86_64.zip -o android-ndk.zip
echo unzipping $NDK...
unzip android-ndk.zip > /dev/null
export NDK_HOME=`pwd`/$NDK
export NDK_HOST=linux-x86_64
make TARGET=android_arm32.release LUAVM=lua51
make TARGET=android_arm32.release LUAVM=lua52
make TARGET=android_arm64.release LUAVM=lua51
make TARGET=android_arm64.release LUAVM=lua52
make TARGET=android_x86.release LUAVM=lua51
make TARGET=android_x86.release LUAVM=lua52
make TARGET=android_x86_64.release LUAVM=lua51
make TARGET=android_x86_64.release LUAVM=lua52