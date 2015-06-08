local am = amulet

local vshader = [[
    attribute float x;
    attribute float y;
    uniform mat4 MVP;
    void main() {
        gl_Position = MVP * vec4(x, y, 0, 1);
    }
]]

local fshader = [[
    precision mediump float;
    uniform vec4 tint;
    void main() {
        vec2 uv = gl_FragCoord.xy / 50.0;
        float r = sin(uv.x) * 0.4 + 0.6;
        float g = cos(uv.x) * 0.4 + 0.6;
        float b = cos(uv.y + uv.x) * 0.4 + 0.6;
        gl_FragColor = vec4( r, g, b, 1.0 ) * tint;
    }
]]

local win = am.window({title = "test1"})

local prog = am.program(vshader, fshader)

local buf = am.buffer(4 * 6)
local xview = buf:view("float", 0, 8)
local yview = buf:view("float", 4, 8)
xview[1] = -0.5
xview[2] = 0
xview[3] = 0.5
yview[1] = -0.4
yview[2] = 0.6
yview[3] = -0.4

local MVP = math.mat4(1)
local base = am.draw_arrays()
    :bind_array("x", xview)
    :bind_array("y", yview)

local group = am.group()

local num_tris = 100
local seeds = {}
for i = 1, num_tris do
    table.insert(seeds, math.random())
end

for i, seed in ipairs(seeds) do
    local size_node = base:scale("MVP")
    local position_node = size_node:translate("MVP")
    local node = position_node
        :bind_vec4("tint", math.random(), math.random(), math.random(), 1)
        :action(function(node)
            local t = am.frame_time
            local r = math.cos(t * seed * 2)
            local x = math.sin((t + seed * 6) * seed) * r 
            local y = math.cos(t - i * seed) * r
            position_node.x = x
            position_node.y = y
            size_node.xy = math.sin(t * seed * 4 + i) * 0.15 + 0.2
            return 0
        end)
    group:append(node)
end

local top = group:bind_mat4("MVP", MVP):bind_program(prog)

top:action(function()
    log(am.perf_stats().avg_fps)
    return 1
end)
:action(function()
    if am.key_pressed.escape then
        win:close()
    end
    return 0
end)

win.root = top
