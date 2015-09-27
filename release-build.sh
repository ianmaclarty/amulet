#!/bin/bash
set -xe

make clean-all

# back up existing settings
if [ -f settings ]; then
    mv settings settings.bak
fi

uname=`uname`
if [ "$uname" = "Linux" ]; then
    os=linux
elif [ "$uname" = "Darwin" ]; then
    os=osx
else
    os=windows
fi

for s in `ls release-settings-$os/settings.*`; do
    cp $s settings && make
done

if [ -f settings.bak ]; then
    mv settings.bak settings
fi
