
![](images/screenshot7.jpg)

# Lua primer {#lua-primer}

Amulet code is written in [Lua](http://www.lua.org).
The version of Lua used by default is LuaJIT-2 on Windows, Mac and Linux
and Lua-5.1 on all other platforms.

What follows is a quick introduction to Lua. For more detail please
see the
[Lua manual](http://www.lua.org/manual/5.1/).

**Comments:**

~~~ {.lua}
-- This is a single line comment

--[[ This is
a multi-line
comment ]]
~~~

**Local variables (block scope):**

~~~ {.lua}
local x = 3.14 -- a number
local str = "a string"
local str2 = [[
a
multi-line
string]]
local bool = true -- a bool, can also be false
local y = nil -- nil value
local v1, v2 = 1, 2 -- create two local variables at once
~~~

**Global variables:**

~~~ {.lua}
score = 0
title = "My Game"
~~~

**If-then-else:**

~~~ {.lua}
local x = 2
if x > 1 then
    print("x > 1")
elseif x > 0 then
    print("0 < x <= 0")
else
    print("x <= 0")
end
~~~

The else part of an if-then-else executes
only if the condition evaluates to `false` or `nil`.

**While loop:**

~~~ {.lua}
local n = 5
while n > 0 do
    print(n)
    n = n - 1
end
-- prints 5 4 3 2 1
~~~

**Repeat-until loop:**

~~~ {.lua}
local n = 0
repeat
    n = n + 1
    print(n)
until n == 5
-- prints 1 2 3 4 5
~~~

**For loop:**

~~~ {.lua}
for i = 1, 5 do
    print(i)
end
-- prints 1 2 3 4 5
for j = 10, 1, -2 do
    print(j)
end
-- prints 10 8 6 4 2
~~~

You can break out of a loop using the `break` statement:

~~~ {.lua}
for j = 1, 10 do
    print(j)
    if j == 5 then
        break
    end
end
-- prints 1 2 3 4 5
~~~

**Operators:**

The arithmetic operators are: `+`, `-`, `*`, `/`, `^` (exponent) and `%` (modulo).

Note that Amulet also overloads the `^` operator for constructing scene graphs
(see [here](#scene-graph-syntax) for more details).

The relational operators are: `==`, `~=` (not equal), `<`, `>`, `<=` and `>=`

The logical operators are: `and`, `or` and `not`.

The string concatenation operator is two dots (e.g. `"abc".."def"`).

**Tables:**

Tables are the only data structure in Lua.
They can be used as key-value maps or arrays.

Keys and values can be of any type except `nil`.

~~~ {.lua}
local t = {} -- create empty table
t["score"] = 10
t[1] = "foo"
t[true] = "x"

-- this creates a table with 2 string keys:
local t2 = {foo = "bar", baz = 123}
~~~

Special syntax is provided for string keys:

~~~ {.lua}
local t = {}
t.score = 10
print(t.score)
~~~

The `#` operator returns the length of an array and array indices start at 1 by
default.

~~~ {.lua}
local arr = {3, 4, 5}
for i = 1, #arr do
    print(arr[i])
end
-- prints 3, 4, 5
table.insert(arr, 6) -- appends 6 to end of arr
table.remove(arr, 1) -- removes the first element of arr
-- arr is now {4, 5, 6}
~~~

Setting a key's value to `nil` removes the key
and indexing a missing key returns `nil`.

You can iterate over all key/value pairs using the `pairs` function.
Note that the order is not preserved.

~~~ {.lua}
local t = {a = 1, b = 2, c = 3}
for k, v in pairs(t) do
    print(k..":"..v)
end
-- prints a:1 c:3 b:2
~~~

To iterate over an array table, keeping the order, use `ipairs`:

~~~ {.lua}
local arr = {"a", "b", "c"}
for k, v in ipairs(arr) do
    print(k..":"..v)
end
-- prints 1:a 2:b 3:c
~~~

**Functions:**

~~~ {.lua}
function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n-1)
    end
end
print(factorial(3)) -- prints 6
~~~

Functions are values in Lua so they can be assigned to variables:

~~~ {.lua}
local add = function(a, b)
    return a + b
end
print(add(1, 2)) -- prints 3
~~~

Special syntax is provided for adding functions
to tables:

~~~ {.lua}
local t = {}
function t.say_hello()
    print("hello")
end
~~~

This is equivalent to:

~~~ {.lua}
local t = {}
t.say_hello = function()
    print("hello")
end
~~~

Special syntactic sugar allows you to use function fields
like methods in object oriented languages:

The code:

~~~ {.lua}
function t:f()
    ...
end
~~~

is equivalent to:

~~~ {.lua}
function t.f(self)
    ...
end
~~~

and the code:

~~~ {.lua}
t:f()
~~~

is equivalent to:

~~~ {.lua}
t.f(t)
~~~

For example:

~~~ {.lua}
local t = {x = 3}
function t:print_x()
    print(self.x)
end
t:print_x()   -- prints 3
t.x = 4
t:print_x()   -- prints 4
~~~

If a function call has only a single string or
table argument, the parentheses can be omitted:

~~~ {.lua}
local
function append_z(str)
    return str.."z"
end
print(append_z"xy") -- prints xyz

local
function sum(values)
    local total = 0
    for _, value in ipairs(values) do
        total = total + value
    end
    return total
end
print(sum{3, 1, 6}) -- prints 10
~~~

Functions may also return multiple values:

~~~ {.lua}
function f()
    return 1, 2
end
local x, y = f()
print(x + y) -- prints 3
~~~

-------------------

![](images/screenshot6.jpg)
