#!/bin/bash
set -xe

mkdir -p platbins
rm -rf platbins/*

for plat in `cd builds && ls`; do
    mkdir platbins/$plat
    cp builds/$plat/release/bin/* platbins/$plat/
done
