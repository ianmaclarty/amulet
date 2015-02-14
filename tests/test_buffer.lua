local
function printvec(v)
    local str = "<"..string.format("%0.2f", v.x)..", "
        ..string.format("%0.2f", v[2])
    if v.z then
        str = str..", "..string.format("%0.2f", v.b)
    end
    if v.w then
        str = str..", "..string.format("%0.2f", v.q)
    end
    str = str..">"
    print(str)
end

local
function printv_and_restore(view, index, val)
    local old = view[index]
    view[index] = val
    printvec(view[index])
    view[index] = old
end

local n = 10000
local buf = amulet.buffer(4 * n)
local view = buf:view("float", 0, 4)

for i = 1, n do
    view[i] = i
end

local total1 = 0
local total2 = 0
for i = 1, n do
    total1 = total1 + view[i]
    total2 = total2 + i
end

assert(total1 == total2)
print(total2)

view:set_nums({1, 2, 3})
print(view[1])
print(view[3])

local view2 = buf:view("vec2", 0, 8)
printvec(view2[1])
printvec(view2[1000])
printv_and_restore(view2, 10, math.vec2(-1, -2))

local view3 = buf:view("vec3", 0, 12)
printvec(view3[1])
printvec(view3[1000])
printv_and_restore(view3, 10, math.vec3(-1, -2, -3))

local view4 = buf:view("vec4", 0, 16)
printvec(view4[1])
printvec(view4[1000])
printv_and_restore(view4, 10, math.vec4(-1, -2, -3, -4))

view:set_nums({100, 200, 300})
print(view[1])
print(view[3])

view2:set_nums({101, 201, 301, 401}, 2)
printvec(view2[2])
printvec(view2[3])

view3:set_nums({102, 202, 302, 402, 502, 602}, 1)
printvec(view3[1])
printvec(view3[2])

view4:set_nums({103, 203, 303, 403, 503, 603, 703, 803}, 3)
printvec(view4[3])
printvec(view4[4])

local buf2 = amulet.buffer(10)
local viewub = buf2:view("ubyte", 0, 1)
for i = 1, 10 do
    viewub[i] = i
end
print(viewub[1])
print(viewub[10])
viewub[2] = 300
print(viewub[2])
viewub[3] = -20
print(viewub[3])

local viewubn = buf2:view("ubyte_norm", 0, 1, 10)
print(viewubn[1] * 255)
print(viewubn[10] * 255)
viewubn[2] = 0.5
print(viewubn[2] * 255)
print(viewub[2])

local viewbyte = buf2:view("byte", 0, 1, 1)
viewbyte[1] = -123
print(viewbyte[1])

local viewbyten = buf2:view("byte_norm", 0, 1, 2)
viewbyten[2] = -123/127
print(math.floor(viewbyten[2]*127))

local viewushort = buf2:view("ushort", 0, 2, 5)
viewushort[1] = 60000
print(viewushort[1])

local viewshort = buf2:view("short", 0, 2, 5)
viewshort[1] = -20000
print(viewshort[1])

local viewuint = buf2:view("uint", 0, 4, 2)
viewuint[1] = 0xFFFFFFFF
print(viewuint[1])

local viewint = buf2:view("int", 0, 4, 2)
viewint[1] = -9999999
print(viewint[1])

local viewushortn = buf2:view("ushort_norm", 0, 2, 5)
viewushortn[1] = 0.5
print(string.format("%0.2f", viewushortn[1]))

local viewshortn = buf2:view("short_norm", 0, 2, 5)
viewshortn[1] = -0.5
print(string.format("%0.2f", viewshortn[1]))

local viewuintn = buf2:view("uint_norm", 0, 4, 2)
viewuintn[1] = 0.25
print(string.format("%0.2f", viewuintn[1]))

local viewintn = buf2:view("int_norm", 0, 4, 2)
viewintn[1] = -0.25
print(string.format("%0.2f", viewintn[1]))

for _, vs in ipairs({{viewub, viewubn}, {viewushort, viewushortn}, {viewshort, viewshortn}, {viewuint, viewuintn}, {viewint, viewintn}}) do
    vs[1]:set_nums{101, 102}
    print(string.format("%0.2f", vs[1][1]))
    print(string.format("%0.2f", vs[1][2]))
    vs[2]:set_nums{0.25, 0.5}
    print(string.format("%0.2f", vs[2][1]))
    print(string.format("%0.2f", vs[2][2]))
end

print("ok")
