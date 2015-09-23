<p align="center"><img src="logo.png"/></p>

**WORK IN PROGRESS** Amulet isn't ready yet, but I'm aiming to have a 1.0 release
by the end of 2015.

Amulet is a Lua-based toolkit for experimenting with interactive 
graphics and audio.

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

[![Build Status](https://travis-ci.org/ianmaclarty/amulet.svg?branch=master)](https://travis-ci.org/ianmaclarty/amulet)
