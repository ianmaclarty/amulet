local
function print_view(view)
    str = "["
    for i = 1, #view do
        local elem = tostring(view[i])
        str = str..elem
        if i < #view then
            str = str..", "
            if elem:match("\n") then
                str = str.."\n "
            end
        end
    end
    str = str.."]"
    print(str)
end

print("view ops")
do
    print_view(mathv.array("float", {1, 2, 3, 4}) + 1)
    print_view(2 + mathv.array("float", {1, 2, 3, 4}))
    print_view(mathv.array("vec2", {vec2(1, 2), vec2(3, 4)}) + vec2(5, 6))
    print_view(vec2(5, 6) - mathv.array("vec2", {vec2(1, 2), vec2(3, 4)}))
    print_view(vec2(1, 2) * mathv.array("vec2", {vec2(2, 2), vec2(3, 3)}))
    print_view(vec2(2, 4) / mathv.array("vec2", {vec2(2, 4), vec2(1, 0.5)}))
    print_view(vec3(1, 2, 3) * mathv.array("vec3", {vec3(2, 2, 2), vec3(3, 3, 3)}))
    print_view(mathv.array("float", {10, 100, 1000}) * vec2(1, 2))
    print_view(vec3(1, 2, 3) * mathv.array("float", {10, 100, 1000}))
    print_view(mathv.array("vec3", {vec3(1, 2, 3), vec3(4, 5, 6), vec3(7, 8, 9)}) + mathv.array("vec3", {1, 0, 0, 0, 1, 0, 0, 0, 1}))
    print_view(mathv.dot(mathv.array("vec2", {vec2(1, 1), vec2(2, 2)}), mathv.array("vec2", {vec2(3, 3), vec2(4, 4)})))
    print_view(mathv.sign(mathv.array("float", {2, 0, -2})))
    print_view(-mathv.array("float", {1, 2}))
end

print("view generators")
do
    print_view(mathv.range("float", 11, -100, 100))
    print_view(mathv.range("double", 3, 10, 0))
    print_view(mathv.range("byte", 11, 100, -100))
    print_view(mathv.range("ubyte", 5, 0, 40))
    print_view(mathv.range("short", 7, -3000, 3000))
    print_view(mathv.range("ushort", 6, 60000, 10000))
    print_view(mathv.range("int", 2, -1000000, 1000000))
    print_view(mathv.range("uint", 9, 10000000, 90000000))
    -- these work, but we get slight differences on different platforms, possibly due to float formatting differences
    -- print_view(mathv.random("float", 10))
    -- print_view(mathv.random("double", 10, -10, 10))
    -- print_view(mathv.random(am.rand(500), "float", 5, 0, 3))
    -- print_view(mathv.random(am.rand(500), "float", 5, 0, 3))

    print("view shape changers")
    print_view(mathv.cart(mathv.array("ubyte", {1, 10, 100}), mathv.array("ubyte2", {2, 20, 200, 220})))
    print_view(mathv.cart(mathv.array("short", {1, 10, 100}), mathv.array("short", {2, 20, 200})))
    print_view(mathv.cart(mathv.array("float", {1, 10, 100}), mathv.array("float", {2, 20, 200})))
    print_view(mathv.cart(mathv.array("vec2", {1, 2, 10, 20, 100, 200}), mathv.array("float", {3, 30, 300})))
    print_view(mathv.cart(mathv.array("double", {3, 30, 300}), mathv.array("dvec2", {1, 2, 10, 20, 100, 200})))
    print_view(mathv.vec2(mathv.array("float", {1, 2}), 3))
    print_view(mathv.vec4(mathv.array("float", {1, 2}), 3, mathv.array("vec2", {vec2(4, 5), vec2(6, 7)})))
end

print("test matrix views")
do
    local range = mathv.range("float", 3, 1, 3)
    local mview1 = mathv.mat3(range * vec3(1, 0, 0), vec3(0, 1, 0) * range, vec3(0, 0, 1) * range)
    print_view(mview1)
    print_view(mview1 * vec3(1, 2, 3))
    print_view(vec3(1, 2, 3) * mview1)
    print_view(mathv.array("vec4", {1, 2, 3, 4, 5, 6, 7, 8}) * mat4(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2))
    print_view(mat4(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2) * mathv.array("vec4", {1, 2, 3, 4, 5, 6, 7, 8}))
    print_view(mathv.vec4(mathv.array("vec3", {1, 2, 3, 4, 5, 6}), 1) * mat4(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2))
end

print("view swizzle fields")
do
    local view1 = mathv.array("vec2", {vec2(1, 2), vec2(3, 4), vec2(5, 6)})
    print_view(view1.x)
    print_view(view1.y)
    print_view(view1.xy)
    local view2 = mathv.array("vec3", {vec3(1, 2, 3), vec3(4, 5, 6), vec3(7, 8, 9)})
    print_view(view2.xy)
    print_view(view2.z)
    local view3 = mathv.array("vec4", {vec4(1, 2, 3, 4), vec4(5, 6, 7, 8)})
    print_view(view3.xy)
    print_view(view3.yzw)
    print_view(view3.zw)
    print_view(view3.rgba)
    print_view(view3.s)
    print_view(view3.y)
    print_view(view3.z)
    print_view(view3.w)

    view3.x = -1
    print_view(view3)
    view3.yz = mathv.array("vec2", {vec2(10, 20), vec2(30, 40)})
    print_view(view3)
    view3.yzw = {100, 200, 300, 400, 500, 600}
    print_view(view3)
    view3.xy = view3.zw
    print_view(view3)
    view3.xyzw = view3
    print_view(view3)
end

print("view methods")
do
    local view1 = mathv.vec2(mathv.range("float", 4, 1, 4), mathv.range("float", 4, 10, 40))
    view1:add(view1, vec2(1, 2))
    print_view(view1)
    view1:vec2(mathv.range("float", 3, -1, -3), mathv.range("float", 3, 1, 3))
    print_view(view1)
end

print("casting")
do
    local src = mathv.array("dvec3", {1, 2, 3, 4, 5, 6})
    local dst = mathv.array("vec3", 2)
    dst.xyz = src
    print_view(dst)
    src = mathv.array("vec3", {1, 2, 3, 4, 5, 6})
    dst = mathv.array("dvec3", 2)
    dst.xyz = src
    print_view(dst)
    src = mathv.array("vec2", {1, 2, 3, 4})
    dst = mathv.array("byte2", 2)
    dst.xy = src
    print_view(dst)
    src = mathv.array("vec2", {1, 2, 3, 4})
    dst = mathv.array("ushort2", 2)
    dst.xy = src
    print_view(dst)
    src = mathv.array("double", {1, 2, 3})
    dst = mathv.array("int", 3)
    dst.x = src
    print_view(dst)
    print_view(mathv.cast("ubyte2", mathv.array("vec2", {vec2(2, 1), vec2(1, 3)})))
end

print("filter")
do
    local view = mathv.array("float", {9, 3, 7, 1, 6, 10, 2})
    print_view(mathv.filter(view, mathv.lt(5, view)))
    print_view(mathv.filter(view, mathv.lte(view, 6)))
    print_view(mathv.filter(view, mathv.and_(mathv.gt(view, 1), mathv.lte(view, 7))))
    local view2 = mathv.array("float", {9, 2, 8, 1, 7, 11, 2})
    print_view(mathv.filter(view, mathv.eq(view, view2)))
    print_view(mathv.filter(view, mathv.eq(view, 1)))
    print_view(mathv.filter(view, mathv.eq(1, view, 1.1)))
    local iview = mathv.array("int", {9, 2, 8, 1, 7, 12, 4})
    print_view(mathv.filter(iview, mathv.eq(iview % 2, 0)))
    local n = iview:filter(mathv.eq(iview % 2, 1))
    print_view(iview:slice(1, n))
end

print("aggregates")
do
    print(mathv.sum(mathv.array("float", {1, 2, 3})))
    print(mathv.sum(mathv.array("dvec2", {vec2(1, 2), vec2(3, 4)})))
    print(mathv.sum(mathv.array("ushort3", {vec3(1, 2, 3), vec3(4, 5, 6)})))
    print(mathv.sum(mathv.lt(mathv.array("float", {1, 5, 2}), mathv.array("float", {3, 2, 5}))))
    print(mathv.greatest(mathv.array("float", {5})))
    print(mathv.greatest(mathv.array("float", {5, 1, -1, 16, 8})))
    print(mathv.greatest(mathv.array("ubyte4", {vec4(1, 2, 3, 4), vec4(4, 3, 2, 1)})))
    print(mathv.least(mathv.array("short", {5})))
    print(mathv.least(mathv.array("double", {5, 1, -1, 16, 8})))
    print(mathv.least(mathv.array("vec3", {vec3(1, 2, 3), vec3(3, 2, 1)})))
end

print("array of structs")
do
    local arr = mathv.array_of_structs(3, {
        "pos", "vec2",
        "color", "ubyte_norm4",
    })
    arr.pos = {0, 1, 2, 3, 4, 5}
    arr.color = {vec4(0, 1, 0, 1), vec4(1, 0, 1, 1), vec4(1, 1, 0, 1)}
    local slice = arr:slice(2, 1)
    slice.pos = vec2(8, 9)
    print_view(arr.pos)
    slice = arr:slice(1, 2, 2)
    slice.color = {vec4(1, 1, 1, 0), vec4(0, 0, 0, 1)}
    print_view(arr.color)
    local n = arr:filter(mathv.gt(arr.pos.x, 5))
    print_view(arr.pos:slice(1, n))
    print_view(arr.color:slice(1, n))
end

print("struct of arrays")
do
    local arr = mathv.struct_of_arrays(3, {
        "pos", "vec2",
        "color", "ubyte_norm4",
    })
    arr.pos = {0, 1, 2, 3, 4, 5}
    arr.color = {vec4(0, 1, 0, 1), vec4(1, 0, 1, 1), vec4(1, 1, 0, 1)}
    local slice = arr:slice(2, 1)
    slice.pos = vec2(8, 9)
    print_view(arr.pos)
    slice = arr:slice(1, 2, 2)
    slice.color = {vec4(1, 1, 1, 0), vec4(0, 0, 0, 1)}
    print_view(arr.color)
    local n = arr:filter(mathv.gt(arr.pos.x, 5))
    print_view(arr.pos:slice(1, n))
    print_view(arr.color:slice(1, n))
end

print("empty arrays")
do
    local arr = mathv.array("float", {})
    local res = arr + arr
    res = arr * 2
    print_view(arr)
    arr = mathv.array("vec3", 0)
    res = vec3(1) + arr
    res:filter(mathv.gt(mathv.array("float", 0), 1))
    print_view(arr)
    res = mathv.cart(mathv.array("vec2", {}), mathv.array("float", {1, 2, 3}))
    print_view(res)
    res = mathv.cart(mathv.array("float", {1, 2, 3}), mathv.array("vec2", {}))
    print_view(res)
    print(mathv.sum(mathv.array("float", 0, 1)))
    print(mathv.greatest(mathv.array("float", 0)))
    print(mathv.least(mathv.array("float", 0)))
end
