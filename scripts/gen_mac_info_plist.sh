#!/bin/sh
set -e
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
	<string>10.9.0</string>
        <key>CFBundleSupportedPlatforms</key>
        <array>
            <string>MacOSX</string>
        </array>
        <key>LSApplicationCategoryType</key>
        <string>%s</string>
        <key>NSHighResolutionCapable</key>
        <true/>
</dict>
</plist>
EOF
done
