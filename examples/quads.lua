local quads = am.quads(2, {"vert", "vec2", "color", "vec3"})
quads:add_quad{vert = {vec2(-100, 0), vec2(-100, -100),
                       vec2(0, -100), vec2(0, 0)},
               color = {vec3(1, 0, 0), vec3(0, 1, 0),
                        vec3(0, 0, 1), vec3(1, 1, 1)}}
quads:add_quad{vert = {vec2(0, 100), vec2(0, 0),
                       vec2(100, 0), vec2(100, 100)},
               color = {vec3(1, 0, 0), vec3(0, 1, 0),
                        vec3(0, 0, 1), vec3(1, 1, 1)}}
local win = am.window{}
local prog = am.program([[
    precision highp float;
    attribute vec2 vert;
    attribute vec3 color;
    uniform mat4 MV;
    uniform mat4 P;
    varying vec3 v_color;
    void main() {
        v_color = color;
        gl_Position = P * MV * vec4(vert, 0.0, 1.0);
    }
]], [[
    precision mediump float;
    varying vec3 v_color;
    void main() {
        gl_FragColor = vec4(v_color, 1.0);
    }
]])
win.scene = am.use_program(prog) ^ quads
