#!/bin/bash
set -ev
dir=$(readlink -f $(dirname ${BASH_SOURCE[0]}))
for s in `ls $dir/$TRAVIS_OS_NAME/settings.*`; do
    cp $s $dir/../settings && cd $dir/.. && make
done
