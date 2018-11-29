#!/bin/sh
set -e
BuildMachineOSBuild=`sw_vers | grep "BuildVersion" | sed 's/^BuildVersion:.//'`
DTXcodeBuild=`xcodebuild -version | grep "Build version" | sed 's/^Build version //'`
XcodeVer=`xcodebuild -version | grep "Xcode" | sed 's/^Xcode //' | sed 's/^\([0-9]*\.[0-9]*\)\.[0-9]*$/\1/'`
XcodeMagVer=`echo $XcodeVer | sed 's/\.[0-9]*//'`
XcodeMinVer=`printf %0.2f $XcodeVer | sed 's/^[0-9]*\.//'`
DTXcode=`printf %02d $XcodeMagVer`$XcodeMinVer
DTCompiler=com.apple.compilers.llvm.clang.1_0
DTPlatformBuild=`xcodebuild -version -sdk | grep "^MacOSX" -A 8 | grep "^ProductBuildVersion:" | sed 's/^ProductBuildVersion: //'`
DTPlatformName=macosx
DTPlatformVersion=`xcodebuild -version -sdk | grep "^MacOSX" -A 8 | grep "^PlatformVersion:" | sed 's/^PlatformVersion: //'`
SDKVersion=`xcodebuild -version -sdk | grep "^MacOSX" -A 8 | grep "^SDKVersion:" | sed 's/^SDKVersion: //'`
DTSDKBuild=$DTPlatformBuild
DTSDKName=macosx$SDKVersion
if [ -z "$BuildMachineOSBuild" ]; then echo unable to determine BuildMachineOSBuild; exit 1; fi
if [ -z "$DTXcodeBuild" ]; then echo unable to determine DTXcodeBuild; exit 1; fi
if [ -z "$DTXcode" ]; then echo unable to determine DTXcode; exit 1; fi
if [ -z "$DTPlatformBuild" ]; then echo unable to determine DTPlatformBuild; exit 1; fi
if [ -z "$DTPlatformVersion" ]; then echo unable to determine DTPlatformVersion; exit 1; fi
for f in `find builds/osx -name "bin" -type d`; do
cat << EOF > $f/Info.plist.tmpl
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleName</key>
	<string>%s</string>
        <key>CFBundleDisplayName</key>
        <string>%s</string>
	<key>CFBundleDevelopmentRegion</key>
	<string>%s</string>
	<key>CFBundleShortVersionString</key>
	<string>%s</string>
        <key>CFBundleVersion</key>
        <string>%s</string>
	<key>CFBundleIdentifier</key>
	<string>%s</string>
	<key>CFBundleExecutable</key>
	<string>amulet</string>
	<key>CFBundleIconFile</key>
	<string>icon.icns</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>LSMinimumSystemVersion</key>
	<string>10.14</string>
        <key>CFBundleSupportedPlatforms</key>
        <array>
            <string>MacOSX</string>
        </array>
        <key>LSApplicationCategoryType</key>
        <string>%s</string>
        <key>NSHighResolutionCapable</key>
        <true/>
	<key>NSHumanReadableCopyright</key>
	<string>%s</string>

	<key>BuildMachineOSBuild</key>
	<string>${BuildMachineOSBuild}</string>
	<key>DTCompiler</key>
	<string>com.apple.compilers.llvm.clang.1_0</string>
	<key>DTPlatformBuild</key>
	<string>${DTPlatformBuild}</string>
	<key>DTPlatformVersion</key>
	<string>${DTPlatformVersion}</string>
	<key>DTSDKBuild</key>
	<string>${DTSDKBuild}</string>
	<key>DTSDKName</key>
	<string>${DTSDKName}</string>
	<key>DTXcode</key>
	<string>${DTXcode}</string>
	<key>DTXcodeBuild</key>
	<string>${DTXcodeBuild}</string>
</dict>
</plist>
EOF
done
