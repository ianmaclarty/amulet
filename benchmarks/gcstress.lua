local branches = {10, 10, 10, 10, 20}
local num_buffers = 10000
local num_loops = 1000
local num_subloops = 1000

local t = am.current_time()

local
function time(msg)
    local t0 = t
    t = am.current_time()
    print(string.format("%0.3fs [%0.3fs]: %s", t - t0, t, msg))
end

local buffers = {}
for i = 1, num_buffers do
    buffers[i] = am.buffer(i * 12)
end

local b = 1
local
function construct_tree(level)
    if not branches[level] then
        local leaf =
            am.bind{
                verts = buffers[b]:view("vec3", 0, 12),
                uvs = buffers[b]:view("vec2", 0, 8),
                t = 0,
                offset = vec3(0),
            }
            ^ am.draw"triangles"
        b = b % num_buffers + 1
        return leaf
    end
    local node = am.group()
    for i = 1, branches[level] do
        node:append(construct_tree(level + 1))
    end
    return 
        am.translate("MV", vec3(0))
        ^ am.rotate("MV", quat(0))
        ^ am.scale("MV", vec3(1))
        ^ node:tag("group")
end

time("start")

local root = construct_tree(1)
time("build tree")

local max_t, min_t = 0, 1000

for i = 1, num_loops do
    local t0 = am.current_time()
    for j = 1, num_subloops do
        local node = root
        for l = 1, #branches do
            local c = math.random(branches[l])
            node = node"group":child(c)
            if l < #branches then
                node"translate".position = vec3(0) + vec3(0)
                node"rotate".rotation = quat(1) * quat(-1)
                node"scale".scale = node"scale".scale * vec3(1)
            else
                node"bind".verts = buffers[math.random(num_buffers)]:view("vec3", 0, 12)
                node"bind".uvs = buffers[math.random(num_buffers)]:view("vec2", 0, 8)
                node"bind".t = i
                node"bind".offset = vec3(1) + vec3(2) + vec3(4)
            end
        end
    end
    local t = am.current_time()
    local diff = t - t0
    if diff > max_t then
        max_t = diff
    end
    if diff < min_t then
        min_t = diff
    end
end

time("mutate")

print(string.format("min loop t = %0.3fs, max loop t = %0.3fs", min_t, max_t))
