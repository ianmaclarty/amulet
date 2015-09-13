Introduction
============

Amulet is a toolkit for experimenting with interactive computer graphics and
audio.
It provides a Lua API for drawing graphics, playing audio and
responding to user input.

Amulet tries to be simple to use, but without making too many assumptions about
how you want to use it.

Amulet currently works on the following platforms:

- Windows 7+ (32 bit)
- Mac OS X 10.6+ (64 bit)
- Linux (32 and 64 bit)
- iOS (armv7 and arm64)
- HTML5

How to use this document
------------------------

The sections in this document are designed to be read in order
as later sections will make use of concepts described in earlier
sections. 

At the end of each section is an API reference listing the available
functions relevant to that section. You can find a list of all available
functions in the `Index <genindex.html>`__.

..  note::

    Most Amulet functions exist in a Lua module called ``amulet``. Sometimes
    in this document you might see ``am`` used instead (for example something
    like ``am.window{}`` instead of ``amulet.window{}``. You can either just use
    the full module name in your code (i.e. ``amulet.window{}``) or do what I do
    and put ``local am = amulet`` at the top of your scripts, which will then
    let you use ``am`` instead.
