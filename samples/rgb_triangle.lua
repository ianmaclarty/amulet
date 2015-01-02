local am = amulet

local vshader = [[
    attribute float x;
    attribute float y;
    attribute float r;
    attribute float g;
    attribute float b;
    varying float r_;
    varying float g_;
    varying float b_;
    void main() {
        r_ = r;
        g_ = g;
        b_ = b;
        gl_Position = vec4(x, y, 0, 1);
    }
]]

local fshader = [[
    precision mediump float;
    varying float r_;
    varying float g_;
    varying float b_;
    void main() {
        gl_FragColor = vec4(r_, g_, b_, 1);
    }
]]

local win = am.window({title = "rgb_triangle", width = 640, height = 480})

local prog = am.program(vshader, fshader)

local stride = 4 * 5
local buf = am.buffer(stride * 3)
local xview = buf:view("float", 0, stride)
local yview = buf:view("float", 4, stride)
local rview = buf:view("float", 8, stride)
local gview = buf:view("float", 12, stride)
local bview = buf:view("float", 16, stride)
xview[1] = -0.5
xview[2] = 0
xview[3] = 0.5
yview[1] = -0.4
yview[2] = 0.6
yview[3] = -0.4
rview[1] = 1
rview[2] = 0
rview[3] = 0
gview[1] = 0
gview[2] = 1
gview[3] = 0
bview[1] = 0
bview[2] = 0
bview[3] = 1

local node = am.draw_arrays()
    :bind_array("x", xview)
    :bind_array("y", yview)
    :bind_array("r", rview)
    :bind_array("g", gview)
    :bind_array("b", bview)
    :bind_program(prog)

win.root = node
