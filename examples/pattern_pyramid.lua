local win = am.window({title = "demo", width = 640, height = 480, resizable = true})

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
        gl_FragColor = texture2D(tex1, uv).rrra;
    }
]]

local prog = am.program(vshader, fshader)

local verts = am.buffer(4 * 6):view("vec2", 0, 8)
verts[1] = math.vec2(-0.8, -0.7)
verts[2] = math.vec2(0, 0.7)
verts[3] = math.vec2(0.8, -0.7)

local n = 16
local tbuf = am.buffer(n^2 * 4)
local tview = tbuf:view("ubyte", 0, 4)
for i = 1, n^2 do
    tview[i] = math.random() * 255
end
local texture = am.texture2d(am.image_buffer(tbuf, n))
texture.filter = "linear"
texture.wrap = "mirrored_repeat"

local node = 
    am.use_program(prog)
    ^am.bind{
        vert = verts,
        tex1 = texture,
    }
    ^am.draw("triangles")

node:action(function()
    tview[math.random(n^2)] = math.random() * 255
end)

win.scene = node
