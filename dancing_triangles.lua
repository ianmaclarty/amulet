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

local win = am.window({title = "test1", highdpi = true})

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
local base = 
    am.bind{
        x = xview,
        y = yview
    }
    ^am.draw("triangles")

local group = am.group()

local num_tris = 100
local seeds = {}
for i = 1, num_tris do
    table.insert(seeds, math.random())
end

for i, seed in ipairs(seeds) do
    local size_node = am.scale("MVP", vec3(1)) ^ base
    local position_node = am.translate("MVP", vec3(0)) ^ size_node
    local node = 
        am.bind{tint = vec4(math.random(), math.random(), math.random(), 1)}
            :action(function(node)
                local t = am.frame_time
                local r = math.cos(t * seed * 2)
                local x = math.sin((t + seed * 6) * seed) * r 
                local y = math.cos(t - i * seed) * r
                position_node.position = vec3(x, y, 0)
                local s = math.sin(t * seed * 4 + i) * 0.15 + 0.2
                size_node.scale = vec3(s, s, 1)
            end)
        ^position_node
    group:append(node)
end

local top = am.use_program(prog) ^ am.bind{MVP = MVP} ^ group

top:action(function()
    if win:key_pressed("escape") then
        win:close()
    end
    if win:key_pressed("f") then
        win.mode = win.mode == "fullscreen" and "windowed" or "fullscreen"
    end
end)

win.scene = top
