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
        gl_FragColor = tint;
    }
]]

local win = am.window({title = "tween test"})

local prog = am.program(vshader, fshader)

local buf = am.buffer(4 * 6)
local xview = buf:view("float", 0, 8)
local yview = buf:view("float", 4, 8)
xview[1] = -0.05
xview[2] = 0
xview[3] = 0.05
yview[1] = -0.05
yview[2] = 0.05
yview[3] = -0.05

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

local e = am.easings

local easings = {
    e.linear,
    e.quadratic,
    e.reverse(e.quadratic),
    e.cubic,
    e.reverse(e.cubic),
    e.onetwo(e.cubic, e.reverse(e.cubic)),
    e.onetwo(e.hyperbola, e.reverse(e.hyperbola)),
    e.sine,
    e.reverse(e.windup),
    e.bounce,
    e.onetwo(e.reverse(e.elastic), e.elastic)
}
local group = am.group()
for i, easing in ipairs(easings) do
    local y = #easings == 1 and 0 or - (i - 1) / (#easings - 1) * 1.6 + 0.8
    local node = base:scale("MVP"):translate("MVP", -0.5, y)
        :action(am.actions.loop(function() 
            return am.actions.seq{
                am.tween{x = 0.5, time = 1, easing = easings[i]},
                am.tween{x = -0.5, time = 1, easing = e.reverse(easings[i])},
                am.tween{y = -0.5, time = 1, easing = e.reverse(easings[i])},
                am.tween{y = y, time = 1, easing = easings[i]},
            }
        end))
        :bind_vec4("tint", 1, 1, 1, 1)
    group:append(node)
end

local top = group:bind_mat4("MVP", MVP):bind_program(prog)

win.root = top
top:action(function()
    if win:key_pressed("escape") then
        win:close()
    end
    return 0
end)
