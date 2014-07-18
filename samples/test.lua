local am    = require "amulet"
local math  = require "math"
local coroutine  = require "coroutine"

local vshader = [[
    attribute float x;
    attribute float y;
    void main() {
        gl_Position = vec4(x, y, 0, 1);
    }
]]

local fshader = [[
    uniform float brightness;
    void main() {
        vec2 uv = gl_FragCoord.xy;
        gl_FragColor = vec4( 1, uv.x / 640.0, uv.y / 480.0, 1.0 ) * brightness;
    }
]]

local win = am.create_window("hello", 640, 480)

local prog = am.program(vshader, fshader)

local buf = am.buffer(4 * 6)
local xview = buf:float_view(0, 8)
local yview = buf:float_view(4, 8)
xview[1] = -0.5
xview[2] = 0
xview[3] = 0.5
yview[1] = -0.4
yview[2] = 0.6
yview[3] = -0.4

local node = am.node():
    use_program(prog):
    set_float("brightness", 1):
    set_array("x", xview):
    set_array("y", yview):
    draw_arrays(0, 3):
    draw_children()

local buf2 = am.buffer(4 * 6)
local xview2 = buf2:float_view(0, 8)
local yview2 = buf2:float_view(4, 8)
xview2[1] = -0.8
xview2[2] = -0.4
xview2[3] = 0
yview2[1] = -0.8
yview2[2] = 0.2
yview2[3] = -0.8

local child = am.node():
    set_array("x", xview2):
    set_array("y", yview2):
    set_float("brightness", 2.5):
    draw_arrays(0, 3)

node:append(child)

node:action(coroutine.wrap(function()
    for i = 1, 300 do
        yview[1] = yview[1] + 0.01
        xview2[3] = xview2[3] + 0.01
        print(i)
        coroutine.yield(0)
    end
    win:close()
end))

win.root = node
