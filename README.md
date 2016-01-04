<p align="center"><img src="logo.png"/></p>

Amulet is a Lua-based audio-visual toolkit designed for small-scope games and experimentation.

**NOTE**: Amulet is not ready yet, but a initial release is planned for the end of 2015.

Here's what a minimal Hello World! looks like in Amulet:

```lua
am.window{}.scene = am.text("Hello World!")
```

## Supported platforms

It currently runs on the following platforms:

- Windows 7+
- Mac OS X 10.6+
- Linux
- iOS
- HTML5

Android support will also be added at some point.

## Building from source

### Linux

Make sure you have gcc and g++ installed, then do:

```
make
```

### OSX

Install the command line developer tools (clang, clang++), then do:

```
make
```

### Windows

You will need to install [MinGW and MSYS](http://www.mingw.org/)
as well as a MicroSoft Visual C compiler.

You will need to create your own version of msys.bat with something
like the following at the top:

```
call "%VS120COMNTOOLS%vsvars32.bat" >NUL:
```

(VS120COMNTOOLS may be different depending on which version
of Visual Studio you're using.)

This set's up the Visual Studio command line environment.

Then, from the msys shell, do:

```
make
```

### Cross compiling

It's also possible to cross-compile to various platforms (e.g. HTML5, iOS).
See the files appveyor.bat and travis.sh in the scripts directory
for examples of how to do this.

## Credits

Amulet uses or borrows code and ideas from the following projects:

- [Lua](http://lua.org)
- [LuaJIT](http://luajit.org/)
- [SDL2](https://www.libsdl.org)
- [Sean's Tool Box](https://github.com/nothings/stb)
- [KissFFT](http://sourceforge.net/projects/kissfft/)
- [Blink](http://www.chromium.org/blink)
- [ANGLE](https://code.google.com/p/angleproject/)
- [FreeType](http://www.freetype.org/)
- [GLM](https://github.com/g-truc/glm)
- [Emscripten](http://emscripten.org)
- [OpenCV](http://opencv.org/)
- [SimpleGlob](https://github.com/brofield/simpleopt)
- [SFMT](http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/index.html)
- [sfxr](http://www.drpetter.se/project_sfxr.html)

The HTML editor uses code and ideas from:

- [Codemirror](https://codemirror.net/)
- [PuzzleScript](https://github.com/increpare/PuzzleScript) and [Zeedonk](https://github.com/increpare/zeedonk)
- [jQuery](https://jquery.com/)

## Build status

| System |  Status |
|:-------------|:------------|
|Travis (Linux, OSX, iOS, MinGW) | [![Build Status](https://travis-ci.org/ianmaclarty/amulet.svg?branch=master)](https://travis-ci.org/ianmaclarty/amulet) |
|Appveyor (MSVC, Emscripten) | [![Build Status](https://ci.appveyor.com/api/projects/status/tp1ifjl53cy86gyu?svg=true)](https://ci.appveyor.com/project/ianmaclarty/amulet) |
