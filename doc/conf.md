
# Project Configuration {#config}

Project metadata can be specified in a `conf.lua` file
in the same directory as `main.lua`. Information like the icon to
use for Mac and iOS exports, your game title, author and version, 
as well as app signing information for iOS is all specified here.

## Basic metadata

A minimal `conf.lua` might look like this:

~~~ {.lua}
title = "My Game Title"
author = "Your Name"
shortname = "mygame"
version = "1.0.0"
support_email = "support@example.com"
copyright_message = "Copyright © 2019 Your Name."
~~~

`shortname` is used for the name of the executable and in a few other places.
`support_email` is displayed to the user in error messages if provided.
`author` is used to determine where save files go as well as in package metadata.
The other metadata may or may not be included in the generated package metadata,
depending on the target platform.

## Language configuration

~~~ {.lua}
dev_region = "en"
supported_languages = "en,fr,de,ja,zh-Hans,zh-Hant,ko"
~~~

These options specify the main language and supported languages
of the app respectively. [`am.language`](#am.language) will always
return one of the supported languages. Note that not all platforms
currently support this feature.

## Lua version

~~~ {.lua}
luavm = "lua52"
~~~

Specify which version of Lua to use for exported builds. This does not
currently affect which version of Lua is used to run the game from the
command line. Valid values are `"lua51"`, `"lua52"` and `"luajit"`.

## Windows settings

~~~ {.lua}
d3dangle = true
~~~

Use Direct3D on Windows instead of OpenGL. Your GLSL shaders will be translated to
HLSL using the Angle library. Note that MSAA is currently not supported when using
Direct3D.

## Mac settings

~~~ {.lua}
appid_mac = "com.example.myappid"
icon_mac = "assets/icon_mac.png"
~~~

Specify the icon and appid for Mac exports. The icon path is relative to
where you run amulet from.

## iOS settings

~~~ {.lua}
display_name = "My Game"
appid_ios = "com.example.mygameid"
icon_ios_ = "assets/icon.png"
launch_image = "assets/launch_image.png"
orientation = "portrait"
ios_dev_cert_identity = "XXXX123456"
ios_appstore_cert_identity = "XXXX123456"
ios_code_sign_identity = "Apple Distribution"
ios_dev_prov_profile_name = "MyGame Dev Profile"
ios_dist_prov_profile_name = "MyGame App Store Profile"
game_center_enabled = true
icloud_enabled = true
~~~

The above metadata is used when the `-ios-xcode-proj` export option is
given to generate an Xcode project for iOS. All the data is required.

`orientation` can be `"portrait"`, `"landscape"`, `"any"` or 
`"hybrid"` (portrait on iPhone, but landscape on iPad).

`ios_dev_cert_identity` and `ios_appstore_cert_identity` is the code that appears in parenthesis in your certificate name
(e.g. if you certificate name in Key Access is "iPhone Distribution: Your Name (XXXX123456)",
then this value should be `"XXXX123456"`.

`ios_code_sign_identity` is the part that comes before the colon in your distribution certificate
name. Typically this is either `"iPhone Distribution"` or `"Apple Distribution"`. (Use the Keychain Access application in
Utilities to view your certificates.)

`ios_dev_prov_profile_name` and `ios_dist_prov_profile_name` are the names
of your installed development and distribution provisioning profiles respectively.
You can install a provisioning profile by downloading and double clicking it.

## Android settings

~~~ {.lua}
display_name = "My Game"
appid_android = "com.example.mygameid"
icon_android = "assets/icon.png"
orientation = "portrait"
android_app_version_code = "4"
google_play_services_id = "58675281521"
~~~

The above metadata is used when the `-android-studio-proj` export option is
given to generate an Android Studio project. All the data, except `google_play_services_id` is
required (`google_play_services_id` is only required if you want to use Google
Play leaderboards or achievements).

`orientation` can be `"portrait"`, `"landscape"` or `"any"`.

`appid_android` is used for the Java package name of the app. It should not contain dashes.
