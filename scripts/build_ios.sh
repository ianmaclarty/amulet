#!/bin/sh
make TARGET=ios32.release
make TARGET=ios64.release
./scripts/gen_ios_universal.sh
./scripts/gen_ios_info_plist.sh
