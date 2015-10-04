local win = am.window({title = "test", width = 100, height = 100})

win.scene = am.group()
local node1 = am.group()
local node2 = am.group()
local node3 = am.group()
win.scene:append(node1)
node1:append(node2)
node1:append(node3)
node3:append(win.scene) -- cycle

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
node2:late_action(function()
    print("late")
end)

local frame = 1
win.scene:action(function()
    print("------- start frame "..frame)
    if frame == 6 then
        win.scene:update()
        print("DONE 6")
    elseif frame == 7 then
        win:close()
    end
    frame = frame + 1
end)

win.scene:action(coroutine.create(function(node)
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
    coroutine.yield()
    print("A6")
end))
