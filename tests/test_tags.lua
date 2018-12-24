require "format"

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

local node = 
    am.bind{C = vec4(4)}:tag'nodeC'
    ^am.bind{B = vec3(3)}:tag"nodeB"
    ^am.bind{A = vec2(2)}:tag"nodeA"
    ^am.group():tag"base"

node"nodeC".name = "C"
node'nodeC'.fc = "CC"
node[[nodeB]].name = "B"
node"nodeB".fb = "BB"
node"nodeA".name = "A"
node"nodeA".fa = "AA"
node"base".name = "base"

print(node"nodeA".name..":"..node"nodeA".name)
print(node"nodeB".name..":"..node"nodeB".name)
print(node"nodeC".name..":"..node"nodeC".name)
print(node"base".name)

print(node'nodeA'.fa)
print(node"nodeB".fb)
print(node[[nodeC]].fc)

local child, parent = node"nodeB"
print(child.name)
print(parent.name)

print("")
print_graph(node)

local nodeD = am.group()
nodeD.name = "D"
node"base":append(nodeD)
print("")
print_graph(node)
local nodeE = am.group():tag"nodeE"
nodeE.name = "E"
node"nodeB":append(nodeE)
print("")
print_graph(node)
node"nodeB":remove(node"nodeB":child(2))
print("")
print_graph(node)

print("")
printf(node"nodeA".A.x)
printf(node"nodeB".B.rgb.rg.r)
printf(node"nodeC".C.pq[2])

node:replace("nodeA", nodeD)
print_graph(node)
node:remove("nodeB")
print_graph(node)

local cycle = 
    am.bind{C = vec3(3)}:tag"nodeC"
    ^am.bind{B = vec2(2, 2)}:tag"nodeB"
    ^am.group():tag"nodeA"

cycle"nodeC".name = "C"
cycle"nodeC".f3 = {f = 3}
cycle"nodeB".name = "B"
cycle"nodeB".f2 = {f = 2}
cycle"nodeA".name = "A"
cycle"nodeA".f1 = {f = 1}

cycle"nodeA":append(cycle)

print("---")
print_graph(cycle)
print("")
print_graph(cycle"nodeA")
print("")
print_graph(cycle"nodeB")
print("")
print_graph(cycle"nodeC")

print("")
printf(cycle"nodeA".f1.f)
printf(cycle"nodeB".f2.f)
printf(cycle"nodeC".f3.f)
printf(cycle"nodeB""nodeA".f1.f)
printf(cycle"nodeB""nodeB".f2.f)
printf(cycle"nodeB""nodeC".f3.f)

print("")
printf(cycle"nodeA""nodeB".B.xy.t)
printf(cycle"nodeC".C.rrr.y)
