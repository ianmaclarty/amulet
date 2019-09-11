local
function printvec(v)
    print(v)
end

local
function printv_and_restore(view, index, val)
    local old = view[index]
    view[index] = val
    printvec(view[index])
    view[index] = old
end

local
function print_view(view)
    str = "["
    for i = 1, #view do
        str = str..tostring(view[i])
        if i == #view then
            str = str.."]"
        else
            str = str..", "
        end
    end
    print(str)
end

local n = 10000
local buf = am.buffer(4 * n)
local view = buf:view("float")

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

view:set({1, 2, 3})
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

view:set({100, 200, 300})
print(view[1])
print(view[3])

local sl = view2:slice(2)
sl:set({101, 201, 301, 401})
printvec(view2[2])
printvec(view2[3])

view3:set({102, 202, 302, 402, 502, 602})
printvec(view3[1])
printvec(view3[2])

view4:slice(3):set({103, 203, 303, 403, 503, 603, 703, 803})
printvec(view4[3])
printvec(view4[4])
print_view(am.float_array{1, 2, 3, 4, 5, 6, 7, 8}:slice(2, 3, 2))
print_view(am.vec2_array{vec2(1), vec2(2), vec2(3), vec2(4), vec2(5), vec2(6)}:slice(1, nil, 3))

local buf2 = am.buffer(10)
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
viewubn[2] = 127/255
print(viewubn[2] * 255)
print(viewub[2])

local viewbyte = buf2:view("byte", 0, 1, 1)
viewbyte[1] = -123
print(viewbyte[1])

local viewbyten = buf2:view("byte_norm", 0, 1, 2)
viewbyten[2] = -1
print(math.floor(viewbyten[2]*127))

local viewushort = buf2:view("ushort", 0, 2, 5)
viewushort[1] = 60000
print(viewushort[1])

local viewushort_elem = buf2:view("ushort_elem", 0, 2, 5)
viewushort_elem[3] = 1
viewushort_elem[4] = 2^16
print(viewushort_elem[3])
print(viewushort_elem[4])

local viewshort = buf2:view("short", 0, 2, 5)
viewshort[1] = -20000
print(viewshort[1])

local viewuint = buf2:view("uint", 0, 4, 2)
viewuint[1] = 0xFFFFFFFF
print(viewuint[1])

local viewuint_elem = buf2:view("uint_elem", 0, 4, 2)
viewuint_elem[1] = 1
viewuint_elem[2] = 2^32
print(viewuint_elem[1])
print(viewuint_elem[2])

local viewint = buf2:view("int", 0, 4, 2)
viewint[1] = -9999999
print(viewint[1])

local viewushortn = buf2:view("ushort_norm", 0, 2, 5)
viewushortn[1] = 32767/65535
print(string.format("%0.2f", viewushortn[1]))

local viewshortn = buf2:view("short_norm", 0, 2, 5)
viewshortn[1] = -32767/65535
print(string.format("%0.2f", viewshortn[1]))

for _, vs in ipairs({{viewub, viewubn}, {viewushort, viewushortn}, {viewshort, viewshortn}, {viewuint}, {viewint}}) do
    vs[1]:set{101, 102}
    print(string.format("%0.2f", vs[1][1]))
    print(string.format("%0.2f", vs[1][2]))
    if vs[2] then
        vs[2]:set{0.25, 0.5}
        print(string.format("%0.2f", vs[2][1]))
        print(string.format("%0.2f", vs[2][2]))
    end
end

local settest1 = am.float_array({1, 2, 3, 4})
settest1:set(0, 1, 2) -- 0 0 3 4
for i = 1, 4 do
    print(settest1[i])
end
settest1:set({5, 5}, 2) -- 0 5 5 4
print_view(settest1)
settest1:set(settest1, 4) -- 0 5 5 0
print_view(settest1)
settest1:set({7, 8, 9}, 2, 2) -- 0 7 8 0
print_view(settest1)
settest1:set(settest1, 1, 0) -- 0 7 8 0
print_view(settest1)
settest1:set(am.float_array{1, 2, 3, 4}, 2, 2) -- 0 1 2 0
print_view(settest1)

local setvec = am.vec3_array({1, 2, 3, 4, 5, 6})
printvec(setvec[1])
printvec(setvec[2])
setvec:set(vec3(6, 7, 8))
printvec(setvec[1])
printvec(setvec[2])

local vec3view = am.vec3_array{vec3(1, 2, 3), vec3(4, 5, 6)}
printvec(vec3view[1])
printvec(vec3view[2])

local struct_arr = am.struct_array(10, {"a", "ubyte", "b", "vec3", "c", "short", "d", "byte", "e", "byte"})
struct_arr.a:set(23)
struct_arr.b:set(vec3(1, 2, 3))
struct_arr.c:set(24001)
struct_arr.d:set(-55)
struct_arr.e:set(19)
struct_arr_buf = struct_arr.a.buffer
print(struct_arr_buf:view("ubyte", 0, 20)[1])
print(struct_arr_buf:view("vec3", 4, 20)[5])
print(struct_arr_buf:view("short", 16, 20)[7])
print(struct_arr_buf:view("byte", 18, 20)[8])
print(struct_arr_buf:view("byte", 19, 20)[10])

local _, err = pcall(function() 
    local b = am.buffer(16)
    local v = b:view("float", 0, 4)
    b:free()
    v[1] = 1
end)
print(err:gsub("^.*%: ", "").."")

print("ubyte_norm4")
buf2 = am.buffer(12)
local viewubn4 = buf2:view("ubyte_norm4", 0, 4)
for i = 1, 3 do
    viewubn4[i] = vec4(i/255, (i+1)/255, 1, (i+2)/255)
end
printvec(viewubn4[1]*255)
printvec(viewubn4[2]*255)
printvec(viewubn4[3]*255)

print("buffer_pool")
do
    local view = mathv.array("float", {1, 2, 3})
    am.buffer_pool(function()
        view:set(view + 1 - view)
    end)
    print_view(view)

    -- regression test: previously using an empty buffer pool twice after
    -- using a non-empty buffer pool triggered an assertion failure
    am.buffer_pool(function()
    end)
    am.buffer_pool(function()
    end)

    -- nested pool
    am.buffer_pool(function()
        local view1 = mathv.range("float", 3, 7, 9)
        local view2 = mathv.range("float", 3, 0, 2)
        am.buffer_pool(function()
            local tmp = mathv.array("float", {-1, -2, -3})
            view1.x = view1 + view2 + tmp - 2
        end)
        view.x = view1 + 1
    end)
    print_view(view)
end

print("ok")
