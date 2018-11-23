#!/bin/sh
set -e
luavm=$1
if [ "$luavm" = "" ]; then
    luavm=lua51
fi
grade=release
if [ "$2" \!= "" ]; then
    grade=$2
fi
make TARGET=ios32.$grade LUAVM=$luavm
make TARGET=ios64.$grade LUAVM=$luavm
./scripts/gen_ios_universal.sh $luavm
./scripts/gen_ios_info_plist.sh $luavm
