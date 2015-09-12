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

local cycles

local root = am.group()
root.name = "root"
local child1 = am.bind{test = math.vec2(0)} ^ am.group()
child1:remove_all()
child1.name = "child1"
local frame = 0
local child2 = am.group()
child2.name = "child2"
local child3 = am.group()
child3.name = "child3"

local
function do_test1()
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

do_test1()

local
function mk_node(name)
    local node = am.group()
    node.name = name
    return node
end

print_graph(
    mk_node("A")
    ^ mk_node("B")
    ^ { mk_node("C") }
)
print_graph(
    mk_node("A")
    ^ mk_node("B")
    ^ { mk_node("C") }
    ^ mk_node("D")
)
print_graph(
    mk_node("A")
    ^ mk_node("B")
    ^ { mk_node("C"), mk_node("D"), mk_node("E") }
)
print_graph(
    mk_node("A")
    ^ mk_node("B")
    ^ { mk_node("C"), mk_node("D"), mk_node("E") }
    ^ mk_node("F")
)
local nodeC = mk_node("C")
print_graph(
    mk_node("A")
    ^ mk_node("B")
    ^ { nodeC, nodeC }
)
nodeC = mk_node("C")
print_graph(
    mk_node("A")
    ^ nodeC
    ^ { nodeC, nodeC }
)
nodeC = mk_node("C")
print_graph(
    mk_node("A")
    ^ { nodeC, nodeC }
    ^ nodeC
)
nodeC = mk_node("C")
print_graph(
    mk_node("A")
    ^ { nodeC, nodeC }
    ^ mk_node("B")
)
nodeC = mk_node("C")
print_graph(
    mk_node("A")
    ^ { nodeC, nodeC }
    ^ mk_node("B")
    ^ { nodeC, nodeC }
)
print_graph(
    mk_node("A")
    ^ {
        mk_node("B")
        ^ mk_node("C")
        ^ mk_node("D")
        ,
        mk_node("E")
        ^ mk_node("F")
    }
)
print_graph(
    mk_node("A")
    ^ {
        mk_node("B")
        ^ mk_node("C")
        ^ mk_node("D")
        ,
        mk_node("E")
        ^ mk_node("F")
    }
    ^ mk_node("G")
)
print_graph(
    mk_node("A")
    ^ (
        mk_node("B")
        ^ mk_node("C")
    )
    ^ mk_node("D")
)
print_graph(
    (
        mk_node("A")
        ^ mk_node("B")
    )
    ^ { mk_node("C"), mk_node("D") }
)
