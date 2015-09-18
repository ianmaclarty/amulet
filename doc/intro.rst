Introduction
============

Amulet is a Lua-based toolkit for experimenting with interactive computer
graphics and audio.
It provides a flexible cross-platform API for drawing graphics, playing audio
and responding to user input, and a command-line interpretter for running
Amulet applications.

It tries to be simple to use, but without making too many assumptions about
how you want to use it.

Amulet currently runs on the following platforms:

- Windows 7+ (32 bit)
- Mac OS X 10.6+ (64 bit)
- Linux (32 and 64 bit)
- iOS (armv7 and arm64)
- HTML5

I will probably add support for Android at some point as well.

How to use this document
------------------------

The sections in this document are designed to be read in order
as later sections will make use of concepts described in earlier
sections. 

If you're mainly interested in drawing simple 2D graphics, playing audio
and reacting to keyboard input then the :ref:`quick-start` should be
enough to get you going.

The sections after the quick start guide go into more detail about 
advanced usage and may require a little bit of knowledge about
modern computer graphics.

The :ref:`api-ref` contains detailed documentation for all the Lua
functions available in Amulet.

Lua notes
---------

The version of Lua used by default is Lua-5.1, although Amulet
allows you to choose 5.2 or LuaJIT as well if you prefer
(see :ref:`project-conf` for more details). Lua-5.1
was choosen as the default mainly because it has the best
performance when run in web browsers.

If you're new to Lua the `Lua manual <http://www.lua.org/manual/5.1/>`__
is a good place to start.

There are a couple of changes Amulet makes to the default Lua
configuration that you should be aware of.

The first is that the creation of new global variables
is disallowed. Personally I've found it way too easy to forget to type ``local``
before a variable, which then results in a global variable silently being
created. This can cause hard to find scoping bugs and memory leaks.
If you really want to create a global variable you can either do
``rawset(_G, "variable_name", value)`` or if you want the default
Lua behaviour back you can do ``setmetatable(_G, nil)``.

The second difference is in the way the ``require`` function
works. The default Lua package loaders have been removed and
replaced with a custom loader. The loader passes an new empty table into each
module it loads. All exported functions can be added to
this table, instead of creating a new table. If no other value is
returned by the module, the passed in table will be used as the
return value for ``require``.

The passed in export table can be accessed via the ``...`` expression.
Here's a short example:

..  code-block:: lua

    local mymodule = ...

    mymodule.message = "hello"

    function mymodule.print_message()
        print(mymodule.message)
    end

This has two advantages. The first is that it allows cyclic
module imports, e.g. module A requires module B which in turn
requires module A. Amulet will detect the recursion and return A's
(incomplete) export table in B. Then when A has finished initializing,
all its functions will be available in B. This does mean that B can't
call any of A's functions while its initializing, but after initialization
all of A's functions will be available.

The second advantage is that it might make code hot-loading feasible.
This feature isn't implemented yet, but it's on my todo list.

Note that you can still return custom values from modules and they
will be returned by ``require`` as usual.
