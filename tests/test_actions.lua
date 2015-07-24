local am = amulet

local win = am.window({title = "test", width = 100, height = 100})

win.root = am.group()
local node1 = am.group()
local node2 = am.group()
local node3 = am.group()
win.root:append(node1)
node1:append(node2)
node1:append(node3)
node3:append(win.root) -- cycle

local f = 1
node3:action(function()
    print("F"..f)
    f = f + 1
end)
local g = 1
node2:action(function()
    print("G"..g)
    g = g + 1
end)

local frame = 1
win.root:action(function()
    print("------- start frame "..frame)
    frame = frame + 1
end)

win.root:action(coroutine.wrap(function(node)
    print("A1")
    local b = 1
    node:action(function()
        print("B"..b)
        if b == 1 then
            node:action(function()
                print("D1")
                node:action(function()
                    print("E1")
                    return true
                end)
                return true
            end)
        end
        b = b + 1
        return b > 2
    end)
    node:action(function()
        print("C1")
        return true
    end)
    coroutine.yield()
    print("A2")
    coroutine.yield()
    print("A3")
    coroutine.yield()
    print("A4")
    coroutine.yield()
    print("A5")
    win:close()
end))
