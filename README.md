<p align="center"><img src="logo.png"/></p>

Amulet is a Lua-based audio-visual toolkit designed for small-scope games and experimentation.

**NOTE**: Amulet is not ready yet, but a initial release is planned for the end of 2015.

Here's what Hello World looks like in Amulet:

```lua
am.window{}.scene = am.translate(320, 240) ^ am.text("Hello World!")
```

## Supported platforms

It currently runs on the following platforms:

- Windows 7+
- Mac OS X 10.6+
- Linux
- iOS
- HTML5

Android support will also be added at some point.

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

The HTML editor uses code and ideas from:

- [Codemirror](https://codemirror.net/)
- [PuzzleScript](https://github.com/increpare/PuzzleScript) and [Zeedonk](https://github.com/increpare/zeedonk)
- [jQuery](https://jquery.com/)

## Build status

| System |  Status |
|:-------------|:------------|
|Travis (Linux, OSX, iOS, MinGW) | [![Build Status](https://travis-ci.org/ianmaclarty/amulet.svg?branch=master)](https://travis-ci.org/ianmaclarty/amulet) |
|Appveyor (MSVC) | [![Build Status](https://ci.appveyor.com/api/projects/status/tp1ifjl53cy86gyu?svg=true)](https://ci.appveyor.com/project/ianmaclarty/amulet) |
