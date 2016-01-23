
# Amulet require function

The `require` function in Amulet is slightly different from
the default one.
The default Lua package loaders have been removed and replaced with a custom
loader. The loader passes a new empty table into each module it loads.
All exported functions can be added to this table, instead of creating a
new table. If no other value is returned by the module, the passed in
table will be used as the return value for `require`.

The passed in export table can be accessed via the `...` expression.
Here's a short example:

~~~ {.lua}
local mymodule = ...

mymodule.message = "hello"

function mymodule.print_message()
    print(mymodule.message)
end
~~~

If this module is in the file `mymodule.lua`, then it can be
imported like so:

~~~ {.lua}
local mymodule = require "mymodule"

mymodule.print_message() -- prints hello
~~~

This scheme allows cyclic module
imports, e.g. module A requires module B which in turn requires module
A. Amulet will detect the recursion and return A's (incomplete) export
table in B. Then when A has finished initialising, all its functions
will be available in B. This does mean that B can't call any of A's
functions while its initialising, but after initialisation all of A's
functions will be available.

Of course you can still return your own values from modules and they will
be returned by `require` as with the default Lua `require` function.

