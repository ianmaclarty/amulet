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

local view2 = buf:view("float2", 0, 8)
printvec(view2[1])
printvec(view2[1000])
printv_and_restore(view2, 10, math.vec2(-1, -2))

local view3 = buf:view("float3", 0, 12)
printvec(view3[1])
printvec(view3[1000])
printv_and_restore(view3, 10, math.vec3(-1, -2, -3))

local view4 = buf:view("float4", 0, 16)
printvec(view4[1])
printvec(view4[1000])
printv_and_restore(view4, 10, math.vec4(-1, -2, -3, -4))

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

local viewubn = buf2:view("ubyte", 0, 1, 10, true)
print(viewubn[1] * 255)
print(viewubn[10] * 255)
viewubn[2] = 0.5
print(viewubn[2] * 255)
print(viewub[2])

local viewushort = buf2:view("ushort", 0, 2, 5, false)
viewushort[1] = 60000
print(viewushort[1])

local viewshort = buf2:view("short", 0, 2, 5, false)
viewshort[1] = -20000
print(viewshort[1])

local viewint = buf2:view("int", 0, 4, 2, false)
viewint[1] = 9999999
print(viewint[1])

local viewushortn = buf2:view("ushort", 0, 2, 5, true)
viewushortn[1] = 0.5
print(string.format("%0.2f", viewushortn[1]))

local viewshortn = buf2:view("short", 0, 2, 5, true)
viewshortn[1] = -0.5
print(string.format("%0.2f", viewshortn[1]))

local viewintn = buf2:view("int", 0, 4, 2, true)
viewintn[1] = -0.25
print(string.format("%0.2f", viewintn[1]))

print("ok")
