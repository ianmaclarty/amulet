#!/bin/bash
set -ev
dir=$(readlink -f $(dirname ${BASH_SOURCE[0]}))
if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    apt-get update -qq
    apt-get install -y libc6-dev-i386 g++-multilib
    apt-get install -y libasound2-dev
    apt-get install -y libasound2-dev:i386
    apt-get install -y mingw-w64
    apt-get install -y python2.7 nodejs default-jre
fi
