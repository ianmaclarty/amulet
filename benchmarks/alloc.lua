local v = vec3(0)
local n = 4000000
for i = 1, n do
    v = v + ((vec4(1) - vec4(1)) * vec4(0) * mat4(1))[1]
end
log(v)
