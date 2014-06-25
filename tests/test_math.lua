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
function printmat(m)
    local str = "["
    for row = 1, 4 do
        for col = 1, 4 do
            local elem = m[col*10+row]
            if elem then
                str = str..string.format("%0.2f", elem)
                if m[(col+1)*10+1] then
                    str = str.."  "
                end
            end
        end
        if m[10+row+1] then
            str = str.."\n "
        end
    end
    str = str.."]"
    print(str)
end

local v = math.vec3(1, 2, 3)
printvec(v)
v.x = 3.14
v.y = 6.666
v.z = -12.009
printvec(v)

v = math.vec3(9, math.vec2(10.1, 11.1)) / math.vec3(math.vec2(1.1, 2), 3)
printvec(v)

v = math.vec3(math.vec3(9, 12, 10)) / 3
printvec(v)

v = 2 * math.vec3(1, 2, 3)
printvec(v)

v = 2 * math.vec4(math.vec3(1, 2, 3), math.vec3(4))
printvec(v)

v = 2 * math.vec2(1, 2)
printvec(v)

v = 3.14 + math.vec4(-3)
printvec(v)

print("---")

v = math.vec2(1, 2)
printvec(v.xy)
printvec(v.yx)
printvec(v.yy)

v = math.vec3(1, 2, 3)
printvec(v.xy)
printvec(v.zyx)
printvec(v.xxyy)

v = math.vec4(1, 2, 3, 4)
printvec(v.xwwx)
printvec(v.wyx)
printvec(math.vec3(1, v.yz))

v.gb = math.vec2(20, 30)
printvec(v)
v.wx = math.vec2(40, 10)
printvec(v)

v = v.xyz
v.bgr = 0.5 * v.rgb
printvec(v)

v = v.yx * 2
v.xy = math.vec2(1, 2)
printvec(v)

local m = math.mat4(2)
printmat(m)
printvec(m[1])
printvec(m[2])
printvec(m[3])
printvec(m[4])

printvec(math.vec4(1, 2, 3, 4) * m)
printvec(m * math.vec4(1, 2, 3, 4))

m = math.mat4( 1,  2,  3,  4,
               5,  6,  7,  8,
               9, 10, 11, 12,
              13, 14, 15, 16)
printmat(m)
m[2] = math.vec4(50, 60, 70, 80)
m[13] = 9
m[44] = 99
printmat(m)

print(math.dot(math.vec3(1, 2, 3), math.vec3(1, 2, 3)))
printvec(math.cross(math.vec3(3, 2, 1), math.vec3(1, 2, 3)))
print(math.length(math.vec4(1, 0, 0, 0)))
print(math.distance(math.vec2(1, 0), math.vec2(0, 1)))
printvec(math.normalize(math.vec3(8, 4, 2)))
printvec(math.faceforward(math.vec2(1, 2), math.vec2(3, 4), math.vec2(5, 6)))
printvec(math.reflect(math.vec2(1, 2), math.vec2(1, 0)))
printvec(math.refract(math.normalize(math.vec3(1, 2, 3)), math.normalize(math.vec3(4, 5, 6)), 0.5))

printvec(-math.vec2(1, 2))
printmat(-math.mat2(1, 2, 3, 4))
