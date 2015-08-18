local v = vec3(0)
for i = 1, 4000000 do
    v = v + (vec3(1) - vec3(1)) * vec3(0)
end
log(v)
v = vec4(0)
for i = 1, 4000000 do
    v = v + ((vec4(1) - vec4(1)) * vec4(0) * mat4(1))[1]
end
log(v)
