#!/bin/sh
mkdir -p google_ads_tmp
curl -o google_ads_tmp/sdk.zip http://dl.google.com/googleadmobadssdk/googlemobileadssdkios.zip
cd google_ads_tmp
unzip sdk.zip
cp -r `find . -type d -name GoogleMobileAds.framework` ../third_party/
cd ..
rm -rf google_ads_tmp
