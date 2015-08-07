local am = amulet

local
function print_graph2(node, visited, indent)
    if visited[node] then
        print(indent..node.name.." [cycle]")
    else
        print(indent..node.name)
        visited[node] = true
        for i, child in node:child_pairs() do
            assert(child == node:child(i))
            print_graph2(child, visited, indent.."  ")
        end
        visited[node] = nil
    end
end

local
function print_graph(root)
    print_graph2(root, {}, "")
end

local node = am.group():alias("base"):alias("name", "base")
    :bind("A", vec2(2)):alias("nodeA"):alias{name = "A", fa = "AA"}
    :bind("B", vec3(3)):alias("nodeB"):alias("name", "B"):alias("fb", "BB")
    :bind("C", vec4(4)):alias("nodeC"):alias("name", "C"):alias("fc", "CC")

print(node.nodeA.name..":"..node.nodeA.name)
print(node.nodeB.name..":"..node.nodeB.name)
print(node.nodeC.name..":"..node.nodeC.name)
print(node.base.name)

print(node.fa)
print(node.fb)
print(node.fc)

print("")
print_graph(node)
node.base:append(am.group():alias("name", "D"))
print("")
print_graph(node)
node.nodeB:append(am.group():alias("name", "E"):alias("nodeE"))
print("")
print_graph(node)
node.nodeB:remove(node.nodeB:child(2))
print("")
print_graph(node)

print("")
print(node.nodeA.A.x)
print(node.nodeB.B.rgb.rg.r)
print(node.nodeC.C.pq[2])

local cycle = am.group():alias("nodeA"):alias("name", "A"):alias("f1", {f = 1})
    :bind("B", vec2(2, 2)):alias("nodeB"):alias("name", "B"):alias("f2", {f = 2})
    :bind("C", vec3(3)):alias("nodeC"):alias("name", "C"):alias("f3", {f = 3})

cycle.nodeA:append(cycle)

print("---")
print_graph(cycle)
print("")
print_graph(cycle.nodeA)
print("")
print_graph(cycle.nodeB)
print("")
print_graph(cycle.nodeC)

print("")
print(cycle.f1.f)
print(cycle.f2.f)
print(cycle.f3.f)
print(cycle.nodeB.f1.f)
print(cycle.nodeB.f2.f)
print(cycle.nodeB.f3.f)

print("")
print(cycle.nodeA.B.xy.t)
print(cycle.C.rrr.y)
