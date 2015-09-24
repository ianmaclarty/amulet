#!/bin/bash
set -ev
cp travis-linux/settings.mingw32 settings && make
#for s in `ls travis-$TRAVIS_OS_NAME/settings.*`; do
#    cp $s settings && make
#done
