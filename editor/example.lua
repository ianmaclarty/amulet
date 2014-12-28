-- hello welcome to amulet
local am = amulet
local win = am.create_window({title = "demo", width = 640, height = 480})

local vshader = [[
    attribute vec2 vert;
    void main() {
        gl_Position = vec4(vert, 0, 1);
    }
]]

local fshader = [[
    precision mediump float;
    uniform sampler2D tex1;
    void main() {
        float scale = 0.08;
        vec2 uv = vec2(gl_FragCoord.x / 640.0, gl_FragCoord.y / 480.0) / scale;
        gl_FragColor = texture2D(tex1, uv);
    }
]]

local prog = am.program(vshader, fshader)

local verts = am.buffer(4 * 6):view("float2", 0, 8)
verts[1] = math.vec2(-1, -1)
verts[2] = math.vec2(0, 1)
verts[3] = math.vec2(1, -1)

local n = 16
local tbuf = am.buffer(n^2)
local tview = tbuf:view("ubyte", 0, 1)
for i = 1, n^2 do
    tview[i] = math.random() * 255
end
tbuf:setup_texture2d(n, n,
    "linear", "linear",
    "mirrored_repeat", "mirrored_repeat",
    "luminance")

local node = am.draw_arrays()
    :bind_array("vert", verts)
    :bind_sampler2d("tex1", tbuf)
    :program(prog)

node:action(function()
    tview[math.random(n^2)] = math.random() * 255
    return 0
end)

win.root = node
