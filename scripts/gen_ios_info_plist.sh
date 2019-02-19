#!/bin/sh
# for launch image sizes see https://wiki.starling-framework.org/manual/ios_launch_images
set -e
luavm=$1
DTXcodeBuild=`xcodebuild -version | grep "Build version" | sed 's/^Build version //'`
XcodeVer=`xcodebuild -version | grep "Xcode" | sed 's/^Xcode //' | sed 's/^\([0-9]*\.[0-9]*\)\.[0-9]*$/\1/'`
XcodeMagVer=`echo $XcodeVer | sed 's/\.[0-9]*//'`
XcodeMinVer=`printf %0.2f $XcodeVer | sed 's/^[0-9]*\.//'`
DTXcode=`printf %02d $XcodeMagVer`$XcodeMinVer
DTCompiler=com.apple.compilers.llvm.clang.1_0
DTPlatformBuild=`xcodebuild -version -sdk | grep "^iPhoneOS" -A 8 | grep "^ProductBuildVersion:" | sed 's/^ProductBuildVersion: //'`
DTPlatformName=iphoneos
DTPlatformVersion=`xcodebuild -version -sdk | grep "^iPhoneOS" -A 8 | grep "^PlatformVersion:" | sed 's/^PlatformVersion: //'`
DTSDKBuild=$DTPlatformBuild
DTSDKName=iphoneos$DTPlatformVersion
if [ -z "$DTXcodeBuild" ]; then echo unable to determine DTXcodeBuild; exit 1; fi
if [ -z "$DTXcode" ]; then echo unable to determine DTXcode; exit 1; fi
if [ -z "$DTPlatformBuild" ]; then echo unable to determine DTPlatformBuild; exit 1; fi
if [ -z "$DTPlatformVersion" ]; then echo unable to determine DTPlatformVersion; exit 1; fi

#echo -DAM_DTXcodeBuild=$DTXcodeBuild -DAM_DTXcode=$DTXcode -DAM_DTCompiler=$DTCompiler -DAM_DTPlatformBuild=$DTPlatformBuild -DAM_DTPlatformName=$DTPlatformName -DAM_DTPlatformVersion=$DTPlatformVersion -DAM_DTSDKBuild=$DTSDKBuild -DAM_DTSDKName=$DTSDKName

for f in `find builds/ios/$luavm -name "bin" -type d`; do
cat << EOF > $f/Info.plist
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
	<key>CFBundleIconFile</key>
	<string></string>
	<key>CFBundleIconFiles</key>
	<array>
		<string>icon152.png</string>
		<string>icon76.png</string>
		<string>icon120.png</string>
		<string>icon57.png</string>
		<string>icon114.png</string>
		<string>icon72.png</string>
		<string>icon144.png</string>
		<string>icon180.png</string>
		<string>icon80.png</string>
		<string>icon40.png</string>
	</array>
	<key>CFBundleIcons</key>
	<dict/>
	<key>CFBundleIcons~ipad</key>
	<dict/>
	<key>CFBundleShortVersionString</key>
	<string>%s</string>
	<key>CFBundleVersion</key>
	<string>%s</string>

	<key>MinimumOSVersion</key>
	<string>6.0</string>

	<key>UIRequiresFullScreen</key>
	<string>YES</string>

	<key>DTCompiler</key>
	<string>${DTCompiler}</string>
	<key>DTPlatformBuild</key>
	<string>${DTPlatformBuild}</string>
	<key>DTPlatformName</key>
	<string>${DTPlatformName}</string>
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

	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>UIDeviceFamily</key>
	<array>
		<integer>1</integer>
		<integer>2</integer>
	</array>
	<key>CFBundleSupportedPlatforms</key>
	<array>
	    <string>iPhoneOS</string>
	</array>
	<key>CFBundleExecutable</key>
	<string>%s</string>
	<key>CFBundleIdentifier</key>
	<string>%s</string>
	<key>LSRequiresIPhoneOS</key>
	<true/>
	<key>UIPrerenderedIcon</key>
	<true/>
	<key>UIRequiredDeviceCapabilities</key>
	<array>
		<string>gamekit</string>
		<string>armv7</string>
	</array>
	<key>UIStatusBarHidden</key>
	<true/>

        <!-- needed for Google Ads -->
        <key>NSAppTransportSecurity</key>
        <dict>
            <key>NSAllowsArbitraryLoads</key>
            <true/>
        </dict>

        <!-- orientation -->
        %s
	<key>UILaunchImages</key>
	<array>
		<dict>
			<key>UILaunchImageMinimumOSVersion</key>
			<string>7.0</string>
			<key>UILaunchImageName</key>
			<string>Default</string>
			<key>UILaunchImageOrientation</key>
			<string>Portrait</string>
			<key>UILaunchImageSize</key>
			<string>{320, 480}</string>
		</dict>
		<dict>
			<key>UILaunchImageMinimumOSVersion</key>
			<string>7.0</string>
			<key>UILaunchImageName</key>
			<string>Default</string>
			<key>UILaunchImageOrientation</key>
			<string>Landscape</string>
			<key>UILaunchImageSize</key>
			<string>{320, 480}</string>
		</dict>
		<dict>
			<key>UILaunchImageMinimumOSVersion</key>
			<string>7.0</string>
			<key>UILaunchImageName</key>
			<string>Default-568h</string>
			<key>UILaunchImageOrientation</key>
			<string>Portrait</string>
			<key>UILaunchImageSize</key>
			<string>{320, 568}</string>
		</dict>
		<dict>
			<key>UILaunchImageMinimumOSVersion</key>
			<string>7.0</string>
			<key>UILaunchImageName</key>
			<string>Default-568h</string>
			<key>UILaunchImageOrientation</key>
			<string>Landscape</string>
			<key>UILaunchImageSize</key>
			<string>{320, 568}</string>
		</dict>
		<dict>
			<key>UILaunchImageMinimumOSVersion</key>
			<string>8.0</string>
			<key>UILaunchImageName</key>
			<string>Default-667h</string>
			<key>UILaunchImageOrientation</key>
			<string>Portrait</string>
			<key>UILaunchImageSize</key>
			<string>{375, 667}</string>
		</dict>
		<dict>
			<key>UILaunchImageMinimumOSVersion</key>
			<string>8.0</string>
			<key>UILaunchImageName</key>
			<string>Default-667h</string>
			<key>UILaunchImageOrientation</key>
			<string>Landscape</string>
			<key>UILaunchImageSize</key>
			<string>{375, 667}</string>
		</dict>
		<dict>
			<key>UILaunchImageMinimumOSVersion</key>
			<string>8.0</string>
			<key>UILaunchImageName</key>
			<string>Default-736h</string>
			<key>UILaunchImageOrientation</key>
			<string>Portrait</string>
			<key>UILaunchImageSize</key>
			<string>{414, 736}</string>
		</dict>
		<dict>
			<key>UILaunchImageMinimumOSVersion</key>
			<string>8.0</string>
			<key>UILaunchImageName</key>
			<string>Default-736h</string>
			<key>UILaunchImageOrientation</key>
			<string>Landscape</string>
			<key>UILaunchImageSize</key>
			<string>{414, 736}</string>
		</dict>
		<dict>
			<key>UILaunchImageMinimumOSVersion</key>
			<string>7.0</string>
			<key>UILaunchImageName</key>
			<string>Default-Portrait</string>
			<key>UILaunchImageOrientation</key>
			<string>Portrait</string>
			<key>UILaunchImageSize</key>
			<string>{768, 1024}</string>
		</dict>
		<dict>
			<key>UILaunchImageMinimumOSVersion</key>
			<string>7.0</string>
			<key>UILaunchImageName</key>
			<string>Default-Landscape</string>
			<key>UILaunchImageOrientation</key>
			<string>Landscape</string>
			<key>UILaunchImageSize</key>
			<string>{768, 1024}</string>
		</dict>
		<dict>
			<key>UILaunchImageMinimumOSVersion</key>
			<string>11.0</string>
			<key>UILaunchImageName</key>
			<string>Default-375w-812h</string>
			<key>UILaunchImageOrientation</key>
			<string>Portrait</string>
			<key>UILaunchImageSize</key>
			<string>{375, 812}</string>
		</dict>
	</array>
</dict>
</plist>
EOF
done
