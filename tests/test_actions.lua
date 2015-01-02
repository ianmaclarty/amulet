local am = amulet

local win = am.window({title = "test", width = 100, height = 100})

win.root = am.group()

local frame = 1
win.root:action(function()
    print("------- start frame "..frame)
    return true
end, nil, 0)
win.root:action(function()
    print("-------   end frame "..frame)
    frame = frame + 1
    return true
end, nil, 1000)

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
                end, nil, 2)
            end)
        end
        b = b + 1
        return b <= 2
    end)
    node:action(function()
        print("C1")
    end)
    coroutine.yield(true)
    print("A2")
    coroutine.yield(true)
    print("A3")
    win:close()
end))
