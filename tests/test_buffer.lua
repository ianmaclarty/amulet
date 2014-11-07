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
print("ok")
