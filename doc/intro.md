
Introduction
============

Amulet is a Lua-based toolkit for experimenting with interactive
graphics and audio. It provides a cross-platform API
for drawing graphics, playing audio and responding to user input, and a
command-line interpreter for running Amulet scripts.

It tries to be simple to use, but without making too many assumptions
about how you want to use it.

Amulet currently runs on the following platforms:

-   Windows 7+ (Intel 32 and 64 bit, Direct3D or OpenGL)
-   Mac OS X 10.6+ (Intel 64 bit only, Metal)
-   Linux (Intel 32 and 64 bit, OpenGL)
-   HTML5 (WebAssembly, WebGL)
-   iOS (ARM 64 bit only, Metal)
-   Android (ARM 32 and 64 bit, OpenGLES)

How to use this document
========================

If you have some programming experience, but are
new to Lua then the [Lua primer](#lua-primer) should
bring you up to speed.

The [quickstart tutorial](#quickstart) introduces basic
concepts and walks you through
drawing things on screen, playing audio and responding to user
input.

The [online editor](http://www.amulet.xyz/editor.html) contains
numerous examples you can experiment with from your browser.

If you see a function argument with square brackets around it
in a function signature, then that argument is optional.
