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
local base = 
    am.bind{
        x = xview,
        y = yview,
    }
    ^am.draw("triangles")

local num_tris = 100
local seeds = {}
for i = 1, num_tris do
    table.insert(seeds, math.random())
end

local e = am.ease

local easings = {
    e.linear,
    e.quadratic,
    e.out(e.quadratic),
    e.cubic,
    e.out(e.cubic),
    e.inout(e.cubic, e.quadratic),
    e.inout(e.cubic),
    e.hyperbola,
    e.inout(e.hyperbola),
    e.sine,
    e.windup,
    e.out(e.windup),
    e.bounce,
    e.elastic,
    e.cubic_bezier(0.1, 0.4, 0.6, 0.9),
}
local group = am.group()

local nodes = {}
for i, easing in ipairs(easings) do
    local y = #easings == 1 and 0 or - (i - 1) / (#easings - 1) * 1.6 + 0.8
    local node = am.translate("MVP", vec3(-0.5, y, 0)) ^ base
    group:append(node)
    table.insert(nodes, node)
end

group:action(coroutine.create(function()
    local anis = {}
    while true do
        for i, node in ipairs(nodes) do
            anis[i] = coroutine.create(function() 
                am.wait(am.tween(node, 1, {["position.x"] = 0.5}, easings[i]))
                am.wait(am.delay(math.random()*0.3))
                am.wait(am.tween(node, 1, {position = vec3(-0.5, node.position.y, 0)}, easings[i]))
                am.wait(am.delay(0.5))
            end)
        end
        am.wait(am.parallel(anis))
    end
end))
        
local top = am.use_program(prog) ^ am.bind{tint = vec4(1), MVP = MVP} ^ group

win.scene = top
top:action(function()
    if win:key_pressed("escape") then
        win:close()
    end
end)
