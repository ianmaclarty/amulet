global1 = 1

noglobals()

global1 = global1 + 1

local ok, msg = pcall(function()
    globals2 = 2
end)
assert(not ok)
print(msg)
ok, msg = pcall(function()
    globals1 = globals2 + 1
end)
assert(not ok)
print(msg)
