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
    void main() {
        vec2 uv = gl_FragCoord.xy;
        float brightness = 1.0;
        gl_FragColor = vec4( 1, uv.x / 640.0, uv.y / 480.0, 1.0 ) * brightness;
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

local node = am.draw_arrays()
    :bind_array("x", xview)
    :bind_array("y", yview)
    :program(prog)

win.root = node
