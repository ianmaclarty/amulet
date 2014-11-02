local am    = require "amulet"
local math  = require "math"
local coroutine  = require "coroutine"

local vshader = [[
    attribute float x;
    attribute float y;
    uniform mat4 MVP;
    void main() {
        gl_Position = MVP * vec4(x, y, 0, 1);
    }
]]

local fshader = [[
    uniform vec4 tint;
    void main() {
        vec2 uv = gl_FragCoord.xy;
        gl_FragColor = vec4( 1, uv.x / 640.0, uv.y / 480.0, 1.0 ) * tint;
    }
]]

local win = am.create_window("hello", 640, 480)

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

local MVP1 = math.mat4(0.1)
MVP1:set(4, 4, 1)
local MVP2 = math.mat4(0.15)
MVP2:set(4, 4, 1)
local base = am.draw_arrays()
    :bind_array("x", xview)
    :bind_array("y", yview)
local node1 = base:translate("MVP", math.vec3(1, 0, 0)):alias("position")
    :bind_mat4("MVP", MVP1):bind_vec4("tint", math.vec4(1, 1.5, 0.5, 1)):alias("tint")
local node2 = base:translate("MVP", math.vec3(1, 0, 0)):alias("position")
    :bind_mat4("MVP", MVP2):bind_vec4("tint", math.vec4(0.1, 0.1, 1, 1)):alias("tint")

local group = am.empty()
group:append(node2)
group:append(node1)

local top = group:program(prog)

top:action(function()
    node1.position.xy = 
        math.vec2(math.random() * 1 - 0.5, math.random() * 1 - 0.5) * 20
    node1.tint.rgb = math.vec3(
        math.random(), math.random(), math.random())
    node2.position.xy = 
        math.vec2(math.random() * 1 - 0.5, math.random() * 1 - 0.5) * 20
    node2.tint.rgb = math.vec3(
        math.random(), math.random(), math.random())
    --print("hello "..node1.position.x)
    return 0
end)

win.root = top
