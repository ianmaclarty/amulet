
# Exporting {#exporting}

To generate distribution packages, use the amulet export command like so:

~~~ {.console}
> amulet export [<dir>]
~~~

`<dir>` is the directory containing your `main.lua` file and optional
`conf.lua` file (see [below](#config)). It defaults to the current directory.

This will generate zip package files for Windows, Mac and Linux in the
current directory.
Alternatively you can pass one of the
options `-windows`, `-mac`, `-linux`, `-html`, `-ios-xcode-proj` or `-android-studio-proj`, to
generate packages for a specific platform.

If the `-r` option is given then all subdirectories will also be included
(recursively), otherwise only the files in the given directory are included.

By default all files in the directory with the following extensions will
be included in the export: `.lua`, `.json`, `.png`, `.jpg`, `.ogg`, `.obj`,
`.vert`, `.frag`.  You can include all files with the `-a` option.

All .txt files will also be copied to the generated zip and
be visible to the user when they unzip it. This is intended for `README.txt`
or similar files.

By default packages are exported to the current directory.
The -d option can be used to specify a different directory
(the directory must already exist).

The -o option allows you to specify the complete path (dir + filename) of
the generated package. In this case the -d option is ignored. The -o option
doesn't work if you're exporting multiple platforms at once.

For example the following will export a windows build to `builds/mygame.zip`. It will
look in `src` for the game files, recursively including all files.

~~~ {.console}
amulet export -windows -r -a -o builds/mygame.zip src
~~~

The following will generate mac and linux builds in the `builds` directory, this
time looking for game files in `game` recursively, but only including
recognised files (since no `-a` option is given):

~~~ {.console}
amulet export -mac -linux -r -d builds game
~~~

As a courtesy to the user, the generate zip packages will contain the game
files in a sub-folder. If you instead want the game files to appear in the
root of the zip, use -nozipdir. You might want this if the game will
run from a launcher such as Steam.

If you want to generate only the `data.pak` file for your project 
you can specify the `-datapak` option. It behaves in a similar way to the
platform options, but generates a platform agnostic `data.pak` file
containing your projects Lua files and other assets. You might want to do this
to update a previously generated xcode project without regenerating all the
project files.

The generated zip will also contain an `amulet_license.txt` file
containing the Amulet license as well as the licenses of all third
party libraries used by Amulet. Some of these licenses require that they
be distributed with copies of their libraries, so to comply you should
include amulet_license.txt when you distribute your work. Note that
these licenses do not apply to your work itself.

**IMPORTANT**: Avoid unzipping and re-zipping the generated packages
as you may inadvertently strip the executable marker from
some files, which will cause them not to work on some platforms.
