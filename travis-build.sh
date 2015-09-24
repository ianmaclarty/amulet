#!/bin/bash
set -ev
for s in `ls travis-$TRAVIS_OS_NAME/settings.*`; do
    cp $s settings && make
done
