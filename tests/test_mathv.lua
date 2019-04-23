local
function print_view(view)
    str = "["
    for i = 1, #view do
        local elem = tostring(view[i])
        str = str..elem
        if i == #view then
            str = str.."]"
        else
            str = str..", "
            if elem:match("\n") then
                str = str.."\n "
            end
        end
    end
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
end

print("view generators")
do
    print_view(mathv.range(11, -100, 100))
    print_view(mathv.range(3, 10, 0))
    -- these work, but we get slight differences on different platforms, possibly due to float formatting differences
    --print_view(mathv.random(10))
    --print_view(mathv.random(10, -10, 10))
    --print_view(mathv.random(am.rand(500), 5, 0, 3))
    --print_view(mathv.random(am.rand(500), 5, 0, 3))

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
    local range = mathv.range(3, 1, 3)
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
    local view1 = mathv.vec2(mathv.range(4, 1, 4), mathv.range(4, 10, 40))
    view1:add(view1, vec2(1, 2))
    print_view(view1)
    view1:vec2(mathv.range(3, -1, -3), mathv.range(3, 1, 3))
    print_view(view1)
end
