local am = amulet

local test = 1
local
function print_children(node)
    print(test)
    test = test + 1
    for i, child in node:children() do
        print(child.data.name)
    end
end

local win = am.create_window("test", 100, 100)

local root = am.empty()
root.data = {name = "root"}
local child1 = am.empty()
child1.data = {name = "child1"}
local frame = 0
local child2 = am.empty():action(function()
    frame = frame + 1
    print("*child2 action frame "..frame.."*")
    if frame == 2 then
        win:close()
    end
    return 0
end)
child2.data.name = "child2"
local child3 = am.empty()
child3.data.name = "child3"

local
function mutate_children()
    root:remove_all()
    print_children(root)

    root:append(child1)
    root:append(child2)
    print_children(root)

    root:append(child3)
    print_children(root)

    root:append(child2)
    root:append(child1)
    print_children(root)

    root:remove(child1)
    print_children(root)

    root:replace(child1, child3)
    print_children(root)

    root:prepend(child1)
    print_children(root)

    root:remove_all()
    print_children(root)

    root:prepend(child2)
    root:append(child3)
    root:prepend(child1)
    print_children(root)
end

mutate_children()
print("---")
win.root = root
mutate_children()
