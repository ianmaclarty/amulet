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
        gl_FragColor = texture2D(tex1, uv);
    }
]]

local win = am.window({title = "Image Test", width = 640, height = 480})

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

local texture = am.texture2d("face.jpg")
texture.wrap = "repeat"

local MVP = math.mat4(1)

local node = 
    am.bind{
        x = xview,
        y = yview,
        tex1 = texture,
        MVP = MVP,
    }
    ^am.use_program(prog)
    ^am.draw'triangles'

win.scene = node
