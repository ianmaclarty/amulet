
# Random Number Generation

### am.rand(seed)
Amulet provides its own random number generator called `am.rand`.

It has some advantages over `math.random`:

- you can use it to create multiple, independent random number generators
- it generates the same sequence of numbers for a given seed on all platforms (`math.random` is different on each platform)

Call `am.rand` with a specified seed to get back a function, which you can use to generate random numbers.

Calling this function works just like `math.random`:

~~~ {.lua}
-- creates a new random number generator with the seed
local r = am.rand(seed)

-- gets a number between 0 and 1 (floating-point)
log(r())

-- gets a number between 1 and 5 (integer, both inclusive)
log(r(5))

-- gets a number between 2 and 6 (integer, both inclusive)
log(r(2, 6))
~~~

