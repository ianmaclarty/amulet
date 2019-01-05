#!/bin/sh
set -e
luavm=$1
if [ "$1" = "" ]; then
    luavm=lua51
fi
make TARGET=ios32.release LUAVM=$luavm
make TARGET=ios64.release LUAVM=$luavm
make TARGET=iossim.release LUAVM=$luavm
./scripts/gen_ios_universal.sh $luavm
./scripts/gen_ios_info_plist.sh $luavm
for f in `find builds/iossim/$luavm/release/bin/ -name "*.a"`; do
    cp $f `dirname $f`/sim_`basename $f`
done
