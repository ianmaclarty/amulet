local am    = require "amulet"
local math  = require "math"

local vshader = [[
    attribute vec2 position;
    void main() {
        gl_Position = vec4(position, 0, 1);
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
local position_view = buf:float2_view(0, 8)
position_view[1] = math.vec2(-0.5, -0.4)
position_view[2] = math.vec2( 0.0,  0.6)
position_view[3] = math.vec2( 0.5, -0.4)

local node = am.node():
    use_program(prog):
    set_float("brightness", 1):as("brightness"):
    set_array("position", position_view):
    draw_arrays(0, 3)

win.root = node

local n = 120

for i = 1, n do
    win:draw(win.root)
    win:swap()
    position_view[2] = position_view[2] + math.vec2(0, 0.005)
    node.brightness = node.brightness - 1/n
end

win:close()
