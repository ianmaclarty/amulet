local
function test(val)
    local str = table.tostring(val)
    print(str)
    assert(table.tostring(loadstring("return "..str)()) == str)
end

test({"a", 2, {"c"}})
test({x = 1, y = 2})
test({_my__field__2 = {1, 2, 3}, ABCdef = [[lala
lala"lala]]})
test({1, 2, [4] = 4})
test({f = 1, "b", "c"})
test({[ [[a
b]] ] = {["&!@#$%^&*()'"] = {}}})
test{points = {{x = 1, y = 2}, {x = -2, y = 9}, {x = 1.25, y = -0.5}}}
test{val1 = vec2(1), val2 = vec3(1, 2, 3), more = {vec4(-1, -2, -3, -4), mat4(1)}}
local cycle = {}
cycle.f = cycle
local _, err = pcall(function() test(cycle) end)
print(err:gsub("^.*%: ", "").."")
