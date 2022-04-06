
# Deriving Amulet Types

### am.type(value) {#am.type .func-def}

Using Lua's builtin function `type` to derive the type of various amulet types will often return the unhelpful string `"userdata"`.

You can use `am.type` instead which will often yield a more flexible result.

~~~ {.lua}
local v4 = vec4(1, 0, 0, 1)

local m2 = mat2(
    vec2(50, 50),
    vec2(120, 70)
)

local quat = quat(math.rad(30), vec3(1, 1, 1))

local win = am.window{}

-- normal lua types
local str = "Hi!"
local num = 4.2;
local f = function(x, y)
    return x ^ y;
end

print(am.type(v4))   -- prints "vec4"
print(am.type(m2))   -- prints "mat2"
print(am.type(quat)) -- prints "quat"
print(am.type(win))  -- prints "window"

print(am.type(str))  -- prints "string"
print(am.type(num))  -- prints "number"
print(am.type(f))    -- prints "function"
~~~
