#!/bin/bash
set -e

# linux
mkdir amulet-${GITHUB_TAG}
cp -r builds/linux64/lua51/release/bin/* amulet-${GITHUB_TAG}/
chmod a+x amulet-${GITHUB_TAG}/amulet
cp -r builds amulet-${GITHUB_TAG}/
cp -r templates amulet-${GITHUB_TAG}/
zip -r amulet-${GITHUB_TAG}-linux.zip amulet-${GITHUB_TAG}
rm -rf amulet-${GITHUB_TAG}

# macos
mkdir amulet-${GITHUB_TAG}
cp -r builds/osx/lua51/release/bin/amulet amulet-${GITHUB_TAG}/
chmod a+x amulet-${GITHUB_TAG}/amulet
cp -r builds amulet-${GITHUB_TAG}/
cp -r templates amulet-${GITHUB_TAG}/
zip -r amulet-${GITHUB_TAG}-macos.zip amulet-${GITHUB_TAG}
rm -rf amulet-${GITHUB_TAG}

# windows
mkdir amulet-${GITHUB_TAG}
cp -r builds/msvc64/lua51/release/bin/* amulet-${GITHUB_TAG}/
mv amulet-${GITHUB_TAG}/amulet.exe amulet-${GITHUB_TAG}/amulet-window.exe
mv amulet-${GITHUB_TAG}/amulet-console.exe amulet-${GITHUB_TAG}/amulet.exe
cp -r builds amulet-${GITHUB_TAG}/
cp -r templates amulet-${GITHUB_TAG}/
zip -r amulet-${GITHUB_TAG}-windows.zip amulet-${GITHUB_TAG}
rm -rf amulet-${GITHUB_TAG}
