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
    uniform sampler2D tex1;
    void main() {
        float scale = 0.3;
        vec2 uv = vec2(gl_FragCoord.x / 640.0, gl_FragCoord.y / 480.0) / scale;
        gl_FragColor = texture2D(tex1, uv).rrra;
    }
]]

local win = am.window({title = "Marpat Dorito", width = 640, height = 480})

local prog = am.program(vshader, fshader)

local vbuf = am.buffer(4 * 6)
local xview = vbuf:view("float", 0, 8)
local yview = vbuf:view("float", 4, 8)
xview[1] = -0.8
xview[2] = 0
xview[3] = 0.8
yview[1] = -0.7
yview[2] = 0.7
yview[3] = -0.7

local n = 16
local tbuf = am.buffer(n^2 * 4)
local tview = tbuf:view("ubyte", 0, 4)
for i = 1, n^2 do
    tview[i] = math.random() * 255
end
local img = am.image_buffer(tbuf, n)
local texture = am.texture2d(img)
texture.wrap = "repeat"

local MVP = math.mat4(1)

local node = 
    am.use_program(prog)
    ^am.bind{
        x = xview,
        y = yview,
        tex1 = texture,
        MVP = MVP,
    }
    ^am.draw("triangles")

win.scene = node
