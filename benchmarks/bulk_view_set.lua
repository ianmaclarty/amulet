local t = am.current_time()

local
function time(msg)
    local t0 = t
    t = am.current_time()
    print(string.format("%0.3fs [%0.3fs]: %s", t - t0, t, msg))
end

local m = 100
local n = 1000000
local tn = {}
local tn2 = {}
local tn3 = {}
local tn4 = {}
local tv2 = {}
local tv3 = {}
local tv4 = {}
for i = 1, n do
    tn[i] = i
    tv2[i] = vec2(i)
    tv3[i] = vec3(i)
    tv4[i] = vec4(i)
end
for i = 1, n*2 do
    tn2[i] = i
end
for i = 1, n*3 do
    tn3[i] = i
end
for i = 1, n*4 do
    tn4[i] = i
end

local sview1 = am.buffer(n * 4):view("float")
local sview2 = am.buffer(n * 8):view("vec2")
local sview3 = am.buffer(n * 12):view("vec3")
local sview4 = am.buffer(n * 16):view("vec4")

local view1 = am.buffer(n * 4):view("float")
local view2 = am.buffer(n * 8):view("vec2")
local view3 = am.buffer(n * 12):view("vec3")
local view4 = am.buffer(n * 16):view("vec4")

time("start")

for i = 1, m do
    for j = 1, n do
        view1[j] = tn[j]
    end
end
time("view1 one-at-a-time")

for i = 1, m do
    view1:set(tn)
end
time("view1 bulk from table")

for i = 1, m do
    view1:set(sview1)
end
time("view1 bulk from view")

for i = 1, m do
    for j = 1, n do
        view2[j] = tv2[j]
    end
end
time("view2 one-at-a-time")

for i = 1, m do
    view2:set(tv2)
end
time("view2 bulk from vec table")

for i = 1, m do
    view2:set(tn2)
end
time("view2 bulk from num table")

for i = 1, m do
    view2:set(sview2)
end
time("view2 bulk from view")

for i = 1, m do
    for j = 1, n do
        view3[j] = tv3[j]
    end
end
time("view3 one-at-a-time")

for i = 1, m do
    view3:set(tv3)
end
time("view3 bulk from vec table")

for i = 1, m do
    view3:set(tn3)
end
time("view3 bulk from num table")

for i = 1, m do
    view3:set(sview3)
end
time("view3 bulk from view")

for i = 1, m do
    for j = 1, n do
        view4[j] = tv4[j]
    end
end
time("view4 one-at-a-time")

for i = 1, m do
    view4:set(tv4)
end
time("view4 bulk from vec table")

for i = 1, m do
    view4:set(tn4)
end
time("view4 bulk from num table")

for i = 1, m do
    view4:set(sview4)
end
time("view4 bulk from view")

