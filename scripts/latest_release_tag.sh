#!/bin/sh
git for-each-ref --sort=taggerdate --format '%(refname)' refs/tags | sed 's/refs\/tags\///g' | grep '^v[0-9]\+\.[0-9]\+\.[0-9]\+$' | tail -n 1
