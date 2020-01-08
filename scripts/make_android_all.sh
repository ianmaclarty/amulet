#!/bin/sh
set -x
set -e
make -j2 TARGET=android_arm32.release
make -j2 TARGET=android_arm64.release
make -j2 TARGET=android_x86.release
make -j2 TARGET=android_x86_64.release
