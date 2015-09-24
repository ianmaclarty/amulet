#!/bin/bash
set -ev
dir=$(readlink -f $(dirname ${BASH_SOURCE[0]}))
if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    sudo apt-get update -qq
    sudo apt-get install -y libc6-dev-i386 g++-multilib
    sudo apt-get install -y libasound2-dev
    sudo apt-get install -y libasound2-dev:i386
    sudo apt-get install -y mingw-w64
    sudo apt-get install -y python2.7 nodejs default-jre
fi
