#!/bin/bash
set -xe

mkdir -p platbins
rm -rf platbins/*

for plat in `cd builds && ls`; do
    mkdir platbins/$plat
    for vm in `cd builds/$plat && ls`; do
        cp builds/$plat/$vm/release/bin/* platbins/$plat/
    done
done
