#!/bin/bash
set -e
tags=`git for-each-ref --sort=taggerdate --format '%(refname)' refs/tags | sed 's/refs\/tags\///g' | grep '^v[0-9]\+\.[0-9]\+\.[0-9]\+$' | tail -n 2`
if [ -z "$1" ]; then
    tag1=`echo $tags | sed 's/ v[0-9]*\.[0-9]*\.[0-9]*$//'`
else
    tag1=$1
fi
if [ -z "$2" ]; then
    tag2=`echo $tags | sed 's/^v[0-9]*\.[0-9]*\.[0-9]* //'`
else
    tag2=$2
fi
echo Changes between $tag1 and $tag2:
git log --oneline --reverse --no-decorate "$tag1..$tag2"
