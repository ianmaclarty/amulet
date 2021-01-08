-- video capture works only on OSX and HTML5
local win = am.window({title = "Webcam demo", width = 640, height = 480})

local vshader = [[
    precision highp float;
    attribute vec2 vert;
    attribute vec2 uv;
    varying vec2 v_uv;
    void main() {
        v_uv = uv;
        gl_Position = vec4(vert, 0.0, 1.0);
    }
]]

local fshader = [[
    precision mediump float;
    uniform sampler2D tex;
    varying vec2 v_uv;
    void main() {
        vec4 s = texture2D(tex, v_uv);
        gl_FragColor = fract(s * 20.0);
    }
]]

local prog = am.program(vshader, fshader)

local texture = am.texture2d(640, 480)
texture.filter = "linear"

win.scene = 
    am.bind{
        vert = am.vec2_array{-1, 1, -1, -1, 1, -1, 1, 1},
        uv = am.vec2_array{0, 0, 0, 1, 1, 1, 1, 0},
        tex = texture,
    }
    ^am.use_program(prog)
    ^am.draw('triangles', am.rect_indices())

win.scene:action(function()
    texture:capture_video()
end)

