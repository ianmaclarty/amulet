local win = am.window{
    title = "Mandelbrot",
    letterbox = false}

local vshader = [[
precision highp float;
attribute vec2 vert;
uniform mat4 MV;
uniform mat4 P;
varying vec2 point;
void main() {
    point = vert;
    gl_Position = P * MV * vec4(vert, 0.0, 1.0);
}
]]

local fshader = [[
precision highp float;
varying vec2 point;
uniform float t;
void main() {
    vec2 z = vec2(0.0);
    vec2 f;
    float len;
    for (int i = 0; i < 100; i++) {
        f.x = z.x * z.x - z.y * z.y + point.x;
        f.y = 2.0 * z.x * z.y + point.y;
        z = f;
        len = sqrt(z.x * z.x + z.y * z.y);
        if (len > 2.0) break;
    }
    if (len < 2.0) {
        gl_FragColor = vec4(fract(len/2.0 + t * 0.1), fract(len * 3.0 + t * 0.3), fract(len * 7.0 + t * 0.7), 1);
    } else {
        gl_FragColor = vec4(fract(len - 2.0 + t), fract((len - 2.0 + 0.3 * t) * 3.0), fract((len - 2.0 + 0.1 * t) * 7.0), 1);
    }
}
]]

local prog = am.program(vshader, fshader)

mandelbrot_node =
    am.use_program(prog)
    ^ am.bind{
        P = math.ortho(-2, 2, -2, 2),
        MV = mat4(1),
        color = vec4(1, 0, 0, 1),
        vert = am.rect_verts_2d(-20, -20, 20, 20),
        t = 0,
    }
    ^ am.scale(1)
    ^ am.translate(0, 0)
    ^ am.draw("triangles", am.rect_indices())

mandelbrot_node:action(function()
    local zoom = 0
    if win:key_down"a" then
        zoom = 1
    elseif win:key_down"z" then
        zoom = -1
    end
    local dir = vec2(0)
    if win:key_down"left" then
        dir = vec2(1, 0)
    elseif win:key_down"right" then
        dir = vec2(-1, 0)
    elseif win:key_down"up" then
        dir = vec2(0, -1)
    elseif win:key_down"down" then
        dir = vec2(0, 1)
    end
    
    local p = mandelbrot_node"translate".position2d
    local s = mandelbrot_node"scale".scale2d
    s = s * (1 + zoom * am.delta_time)
    p = p + dir / s * am.delta_time
    mandelbrot_node"translate".position2d = p
    mandelbrot_node"scale".scale2d = s
    mandelbrot_node"bind".t = am.frame_time
end)

win.scene = am.group{
    mandelbrot_node,
    am.translate(0, -200) ^ am.text("ARROW KEYS + AZ", vec4(0, 0, 0, 1))
}
