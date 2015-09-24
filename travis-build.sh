#!/bin/bash
set -ev
make clean-all
cp travis-linux/settings.html settings && make
#for s in `ls travis-$TRAVIS_OS_NAME/settings.*`; do
#    cp $s settings && make
#done
