
# Exporting

To generate distribution packages for Windows, Mac OS X, Linux
and HTML5, use the amulet export command.

To do an export run the following command from the
directory containing you Lua, image and sound files:

~~~ {.console}
> amulet export
~~~

This will generate zip files for each target platform in the
current directory.

All files in the directory with the following extensions will
be included as data in the distribution: `.lua`, `.png`, `.jpg`, `.ogg` and `.obj`.
All .txt files will also be copied to the generated zip and
be visible to the user when they unzip it. This is intended for `README.txt`
or similar files.

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
~~~

then this will be used for various bits of meta-data in the
generated packages. In particular `shortname` will be used
in the name of the generated zip files (otherwise they will
just be called "Untitled").

**IMPORTANT**: Avoid unzipping and re-zipping the generated packages
as you may inadvertently strip the executable bit from
some files, which will cause them not to work. 
