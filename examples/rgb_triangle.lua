local win = am.window{}
local prog = am.program([[
    precision highp float;
    attribute vec2 vert;
    attribute vec4 color;
    uniform mat4 MV;
    uniform mat4 P;
    varying vec4 v_color;
    void main() {
        v_color = color;
        gl_Position = P * MV * vec4(vert, 0.0, 1.0);
    }
]], [[
    precision mediump float;
    varying vec4 v_color;
    void main() {
        gl_FragColor = v_color;
    }
]])
win.scene =
    am.use_program(prog)
    ^ am.bind{
        P = mat4(1),
        MV = mat4(1),
        color = mathv.array("ubyte_norm4", {vec4(1, 0, 0, 1), vec4(0, 1, 0, 1), vec4(0, 0, 1, 1)}),
        vert = mathv.array("vec2", {vec2(-1, -1), vec2(0, 1), vec2(1, -1)}),
    }
    ^ am.draw"triangles"
