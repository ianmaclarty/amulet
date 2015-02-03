local am = amulet

local test = 1
local
function print_graph2(node, visited, indent)
    if visited[node] then
        print(indent..node.name.." [cycle]")
    else
        print(indent..node.name)
        visited[node] = true
        for i = 1, node.num_children do
            local child = node:child(i)
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

local win = am.window({title = "test", width = 100, height = 100})

local cycles

local root = am.group()
root:alias("name", "root")
local child1 = am.group():bind_vec2("test", math.vec2(0))
child1:remove_all()
child1:alias("name", "child1")
local frame = 0
local child2 = am.group():action(function()
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
child2:alias("name", "child2")
local child3 = am.group()
child3:alias{name = "child3"}

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