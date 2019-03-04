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
    print(""..str:gsub("%-0%.00", "0.00"))
end

local
function printquat(q)
    local str = string.format("quat(%0.2f, <%0.2f, %0.2f, %0.2f>)",
        math.deg(q.angle), q.axis.x, q.axis.y, q.axis.z)
    print(""..str:gsub("%-0%.00", "0.00"))
end

local
function printeuler(e)
    local str = string.format("euler(%0.2f, %0.2f, %0.2f)",
        math.deg(e.x), math.deg(e.y), math.deg(e.z))
    print(""..str:gsub("%-0%.00", "0.00"))
end

local
function printmat(m)
    print(m)
end

local v = vec3(1, 2, 3)
printvec(v)
v = v{x = 3.14, y = 6.666, z = -12.009}
printvec(v)

v = vec3(9, vec2(10.1, 11.1)) / vec3(vec2(1.1, 2), 3)
printvec(v)

v = vec3(vec3(9, 12, 10)) / 3
printvec(v)

v = 2 * vec3(1, 2, 3)
printvec(v)

v = 2 * vec4(vec3(1, 2, 3), vec3(4))
printvec(v)

v = 2 * vec2(1, 2)
printvec(v)

v = 3.14 + vec4(-3)
printvec(v)

print("---")

v = vec2(1, 2)
printvec(v.xy)
printvec(v.yx)
printvec(v.yy)

v = vec3(1, 2, 3)
printvec(v.xy)
printvec(v.zyx)
printvec(v.xxyy)

v = vec4(1, 2, 3, 4)
printvec(v.xwwx)
printvec(v.wyx)
printvec(vec3(1, v.yz))

v = v{gb = vec2(20, 30)}
printvec(v)
v = v("wx", vec2(40, 10))
printvec(v)

v = v{xyzw = vec4(1, 2, 3, 4)}
printvec(v)
v = v{rgba = 0.5}
printvec(v)
v = v{stq = 2}
printvec(v)
v = v{s = v.s/2, pq = 3}
printvec(v)
v = v.xyz * 10
v = v{bgr = 0.5 * v.rgb}
printvec(v)

v = v.yx * 2
v = v{xy = vec2(1, 2)}
printvec(v)

print(vec2(1,2) == vec2(1,2))
print(vec2(1,2) == vec2(7,8))

print(vec3(1,2,3) == vec3(1,2,3))
print(vec3(1,2,3) == vec3(7,8,9))

print(vec4(1,2,3,4) == vec4(1,2,3,4))
print(vec4(1,2,3,4) == vec4(7,8,9,0))

print(vec2(1,2) == vec3(1,2,3))

print(tostring(vec2(1,2)))
print(tostring(vec3(1,2,3)))
print(tostring(vec4(1,2,3,4)))
print("concat+"..vec2(1,2))
print("concat+"..vec3(1,2,3))
print("concat+"..vec4(1,2,3,4))
print(vec2(1,2).."+concat")
print(vec3(1,2,3).."+concat")
print(vec4(1,2,3,4).."+concat")

local m = mat4(2)
printmat(m)
printvec(m[1])
printvec(m[2])
printvec(m[3])
printvec(m[4])

printvec(vec4(1, 2, 3, 4) * m)
printvec(m * vec4(1, 2, 3, 4))

m = mat4( 1,  2,  3,  4,
          5,  6,  7,  8,
          9, 10, 11, 12,
         13, 14, 15, 16)
printmat(m)
m = mat4(m[1], vec4(50, 60, 70, 80), m[3], m[4])
m:set(1, 3, 9)
m:set(4, 4, 99)
printmat(m)

printmat(mat2(mat3(1, 2, 3, 4, 5, 6, 7, 8, 9)))

printmat(math.inverse(math.inverse(mat3(1, 2, 1, -1, -2, 1, 1, 0, -3))))
printmat(math.inverse(mat3(1)/mat3(1, 2, 1, -1, -2, 1, 1, 0, -3)))

print(math.dot(vec3(1, 2, 3), vec3(1, 2, 3)))
printvec(math.cross(vec3(3, 2, 1), vec3(1, 2, 3)))
print(math.length(vec4(1, 0, 0, 0)))
print(math.distance(vec2(1, 0), vec2(0, 0)))
printvec(math.normalize(vec3(8, 4, 2)))
printvec(math.faceforward(vec2(1, 2), vec2(3, 4), vec2(5, 6)))
printvec(math.reflect(vec2(1, 2), vec2(1, 0)))
printvec(math.refract(math.normalize(vec3(1, 2, 3)), math.normalize(vec3(4, 5, 6)), 0.5))

printvec(-vec2(1, 2))
printmat(-mat2(1, 2, 3, 4))

print(mat2(1,2,3,4) == mat2(1,2,3,4))
print(mat2(1,2,3,4) == mat2(9,2,3,4))

print(mat3(1,2,3,4,5,6,7,8,9) == mat3(1,2,3,4,5,6,7,8,9))
print(mat3(1,2,3,4,5,6,7,8,9) == mat3(9,2,3,4,5,6,7,8,9))

print(mat4(1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4) == mat4(1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4))
print(mat4(1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4) == mat4(9,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4))

print(mat2(1,2,3,4) == mat3(1,2,3,4,5,6,7,8,9))

print(math.clamp(-1, 0, 1)) -- 0
print(math.clamp(4, 0, 1)) -- 1
print(math.clamp(-1, -3, -1)) -- -1

printvec(math.mix(vec3(0, -1, 2), vec3(1, 1, -1), 0.5)) -- 0.5, 0, 0.5
printvec(math.mix(vec2(0, 100), vec2(100, 0), vec2(0.25, 0.90))) -- 25, 10
print(math.mix(100, 200, 0.1)) -- 110

local q1 = quat(math.rad(90), vec3(0, 0, 1))
local v1 = vec3(1, 0, 0)
printquat(q1 * q1)
printvec(q1 * v1)
printvec(q1 * q1 * v1)
printvec(v1 * q1)
printquat(quat(0, 0, math.rad(90)))
printquat(quat(math.rad(45)))
printquat(quat(vec3(1, 0, 0), vec3(0, 1, 0)))
printquat(quat(q1.w, q1.x, q1.y, q1.z))
printeuler(vec3(q1.pitch, q1.yaw, q1.roll))
printquat(quat(mat3(q1)))
printquat(quat(mat4(q1)))
local v2 = vec2(1, 0)
printvec(q1 * v2)
printvec(v2 * q1)
print(quat(math.rad(90), vec3(0, 0, 1)) == quat(math.rad(90), vec3(0, 0, 1)))
print(quat(math.rad(90), vec3(0, 0, 1)) == quat(math.rad(11), vec3(0, 0, 1)))
print(quat(math.rad(90), vec3(0, 0, 1)) == quat(math.rad(90), vec3(1, 0, 1)))
print(quat(math.rad(90), vec3(0, 0, 1)) == vec4(1,2,3,4))
