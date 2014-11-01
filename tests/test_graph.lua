local am = amulet

local test = 1
local
function print_graph2(node, visited, indent)
    if visited[node] then
        print(indent..node.data.name.." [cycle]")
    else
        print(indent..node.data.name)
        visited[node] = true
        for i, child in node:children() do
            print_graph2(child, visited, indent.."  ")
        end
        visited[node] = nil
    end
end
local
function print_graph(root)
    print(test)
    test = test + 1
    print_graph2(root, {}, "")
end

local win = am.create_window("test", 100, 100)

local cycles

local root = am.empty()
root.data = {name = "root"}
local child1 = am.empty()
child1.data = {name = "child1"}
local frame = 0
local child2 = am.empty():action(function()
    frame = frame + 1
    print("*child2 action frame "..frame.."*")
    if frame == 2 then
        child1:action(function()
            print("*child1 action*")
            win:close()
        end)
        return
    end
    return 0
end)
child2.data.name = "child2"
local child3 = am.empty()
child3.data.name = "child3"

local
function do_test()
    root:remove_all()
    child1:remove_all()
    child2:remove_all()
    child3:remove_all()

    print_graph(root)

    root:append(child1)
    root:append(child2)
    print_graph(root)

    root:append(child3)
    print_graph(root)

    root:append(child2)
    root:append(child1)
    print_graph(root)

    root:remove(child1)
    print_graph(root)

    root:replace(child1, child3)
    print_graph(root)

    root:prepend(child1)
    print_graph(root)

    root:remove_all()
    print_graph(root)

    root:prepend(child2)
    root:append(child3)
    root:prepend(child1)
    print_graph(root)

    -- cycles

    child1:append(child1)
    print_graph(root)

    child2:append(child1)
    print_graph(root)

    child1:remove_all()
    child2:remove_all()
    child3:append(child2)
    child2:prepend(child3)
    child3:append(child1)
    child1:append(child2)
    print_graph(root)

    child1:remove(child2)
    print_graph(root)
end

do_test()
print("---")
win.root = root
do_test()
