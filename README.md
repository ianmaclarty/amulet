<p align="center"><img src="logo.png"/></p>

**WORK IN PROGRESS** Amulet isn't ready yet, but I'm aiming to have a 1.0 release
by the end of 2015.

Amulet is a Lua-based framework for making games by [Ian MacLarty](http://ianmaclarty.com).

Here's what Hello World looks like in Amulet:

```lua
am.window{}.scene = am.camera2d(200, 150, vec2(0)) ^ am.text("Hello World!")
```

It currently runs on the following platforms:

- Windows 7+
- Mac OS X 10.6+
- Linux
- iOS
- HTML5

I will probably add support for Android at some point as well.

Here are some games I've made with Amulet:

- [City Lights](http://ianmaclarty.itch.io/city-lights)
- [Doomdream](http://ianmaclarty.itch.io/doomdream)
- [Southbank Portrait](http://ianmaclarty.itch.io/southbank-portrait)
- [Episode 6](http://forum.makega.me/t/chain-game-2-schedule-games/1483/66?u=muclorty)
- [Zero Sum](http://ianmaclarty.itch.io/zero-sum)
- [Leisurely Sunday Drive IV: the Reckoning](http://gamejolt.com/games/leisurely-sunday-drive-iv-the-reckoning/80716)
- [Reflection](http://ianmaclarty.itch.io/reflection)
- [Totem](http://ludumdare.com/compo/ludum-dare-33/?action=preview&uid=20641)

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
- [GLM](http://glm.g-truc.net)
- [Emscripten](http://emscripten.org)
- [OpenCV](http://opencv.org/)

The HTML editor uses code and ideas from:

- [Codemirror](https://codemirror.net/)
- [PuzzleScript](https://github.com/increpare/PuzzleScript) and [Zeedonk](https://github.com/increpare/zeedonk)
- [jQuery](https://jquery.com/)

[![Build Status](https://travis-ci.org/ianmaclarty/amulet.svg?branch=master)](https://travis-ci.org/ianmaclarty/amulet)
