
# Exporting {#exporting}

To generate distribution packages for Windows, Mac OS X, iOS, Linux
and HTML5, use the amulet export command.

Run the following command from the directory containing your lua, image and
audio files:

~~~ {.console}
> amulet export
~~~

This will generate package files for each supported platform in the
current directory.
If you just want to generate a package for one platform you can
pass one of the options `-windows`, `-mac`, `-ios`, `-linux` or `-html`.

All files in the directory with the following extensions will
be included as data in the distribution: `.lua`, `.json`, `.png`, `.jpg`, `.ogg` and `.obj`.
All .txt files will also be copied to the generated zip and
be visible to the user when they unzip it. This is intended for `README.txt`
or similar files.

If the `-r` option is given then all subdirectories will also be included
(recursively), otherwise only the files in current directory are included.

The generated zip will also contain an `amulet_license.txt` file
containing the Amulet license as well as the licenses of all third
party libraries used by Amulet. Some of these licenses require that they
be distributed with copies of their libraries, so to comply you should
include amulet_license.txt when you distribute your work. Note that
these licenses do not apply to your work itself.

If you create a `conf.lua` file in the same
directory as your other Lua files containing
the following:

~~~ {.lua}
title = "My Game Title"
shortname = "mygame"
author = "Your Name"
appid = "com.some-unique-id.123"
version = "1.0.0"
support_email = "support@example.com"

display_name = "My Game"
dev_region = "en"
supported_languages = "en,fr,nl,de,ja,zh-CN,zh-TW"
icon = "icon.png"
launch_image = "launch.png"
orientation = "any"
~~~

then this will be used for various bits of meta-data in the
generated packages. In particular `shortname` will be used
in the name of the generated zip files (otherwise they will
just be called "Untitled").

If you're generating an iOS package for submission to the App Store
you'll need to fill out all the settings.

Note that the generated iOS ipa package is not signed. You will
need to sign it using a tool like [sigh](https://github.com/fastlane/fastlane/tree/master/sigh)
or [this script](https://raw.githubusercontent.com/fastlane/fastlane/master/sigh/lib/assets/resign.sh)
before submitting it to the app store or deploying on a device.

**IMPORTANT**: Avoid unzipping and re-zipping the generated packages
as you may inadvertently strip the executable bit from
some files, which will cause them not to work. 
