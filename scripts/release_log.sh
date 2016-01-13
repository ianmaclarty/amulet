#!/bin/bash
set -e
tags=`git for-each-ref --sort=taggerdate --format '%(refname)' refs/tags | sed 's/refs\/tags\///g' | grep '^v[0-9]\+\.[0-9]\+\.[0-9]\+$' | tail -n 2`
tag1=`echo $tags | sed 's/ v[0-9]\+\.[0-9]\+\.[0-9]\+$//'`
tag2=`echo $tags | sed 's/^v[0-9]\+\.[0-9]\+\.[0-9]\+ //'`
echo Changes between $tag1 and $tag2:
git log --oneline $tag1..$tag2
