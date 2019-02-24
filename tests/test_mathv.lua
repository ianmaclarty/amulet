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
    print_view(am.float_array{1, 2, 3, 4} + 1)
    print_view(2 + am.float_array{1, 2, 3, 4})
    print_view(am.vec2_array{vec2(1, 2), vec2(3, 4)} + vec2(5, 6))
    print_view(vec2(5, 6) - am.vec2_array{vec2(1, 2), vec2(3, 4)})
    print_view(vec2(1, 2) * am.vec2_array{vec2(2, 2), vec2(3, 3)})
    print_view(vec2(2, 4) / am.vec2_array{vec2(2, 4), vec2(1, 0.5)})
    print_view(vec3(1, 2, 3) * am.vec3_array{vec3(2, 2, 2), vec3(3, 3, 3)})
    print_view(am.float_array{10, 100, 1000} * vec2(1, 2))
    print_view(vec3(1, 2, 3) * am.float_array{10, 100, 1000})
    print_view(am.vec3_array{vec3(1, 2, 3), vec3(4, 5, 6), vec3(7, 8, 9)} + am.vec3_array{vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)})
    print_view(mathv.dot(am.vec2_array{vec2(1, 1), vec2(2, 2)}, am.vec2_array{vec2(3, 3), vec2(4, 4)}))
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
    print_view(mathv.cart(am.float_array{1, 10, 100}, am.float_array{2, 20, 200}))
    print_view(mathv.cart(am.vec2_array{1, 2, 10, 20, 100, 200}, am.float_array{3, 30, 300}))
    print_view(mathv.cart(am.float_array{3, 30, 300}, am.vec2_array{1, 2, 10, 20, 100, 200}))
    print_view(mathv.vec2(am.float_array{1, 2}, 3))
    print_view(mathv.vec4(am.float_array{1, 2}, 3, am.vec2_array{vec2(4, 5), vec2(6, 7)}))
end

print("test matrix views")
do
    local range = mathv.range(3, 1, 3)
    local mview1 = mathv.mat3(range * vec3(1, 0, 0), vec3(0, 1, 0) * range, vec3(0, 0, 1) * range)
    print_view(mview1)
    print_view(mview1 * vec3(1, 2, 3))
    print_view(vec3(1, 2, 3) * mview1)
    print_view(am.vec4_array{1, 2, 3, 4, 5, 6, 7, 8} * mat4(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2))
    print_view(mat4(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2) * am.vec4_array{1, 2, 3, 4, 5, 6, 7, 8})
    print_view(mathv.vec4(am.vec3_array{1, 2, 3, 4, 5, 6}, 1) * mat4(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2))
end

print("view swizzle fields")
do
    local view1 = am.vec2_array{vec2(1, 2), vec2(3, 4), vec2(5, 6)}
    print_view(view1.x)
    print_view(view1.y)
    print_view(view1.xy)
    local view2 = am.vec3_array{vec3(1, 2, 3), vec3(4, 5, 6), vec3(7, 8, 9)}
    print_view(view2.xy)
    print_view(view2.z)
    local view3 = am.vec4_array{vec4(1, 2, 3, 4), vec4(5, 6, 7, 8)}
    print_view(view3.xy)
    print_view(view3.yzw)
    print_view(view3.zw)
    print_view(view3.rgba)
    print_view(view3.s)
    print_view(view3.y)
    print_view(view3.z)
    print_view(view3.w)
end
