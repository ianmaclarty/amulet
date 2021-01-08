-- note: this example only works with luajit

local win = am.window{letterbox = false}

ffi.cdef[[
typedef struct { float x, y; } vec2;
typedef struct { float r, g, b, a; } color;
]]


local n = 200000
local sz = ffi.sizeof("vec2[?]", n)
local vbuf = am.buffer(sz)
local verts = ffi.cast("vec2*", vbuf.dataptr)
local vels = ffi.new("vec2[?]", n)

local csz = ffi.sizeof("color[?]", n)
local cbuf = am.buffer(csz)
local colors = ffi.cast("color*", cbuf.dataptr)

for i = 0, n - 1 do
    local x, y = math.random() - 0.5, math.random() - 0.5
    verts[i].x = x * win.width
    verts[i].y = y * win.height
    vels[i].x = 0
    vels[i].y = 0
    colors[i].r = math.random() ^ 7
    colors[i].g = math.random() ^ 10
    colors[i].b = math.random()
    colors[i].a = 1
end
vbuf:mark_dirty()

local vshader = [[
    precision highp float;
    attribute vec2 vert;
    attribute vec4 color;
    uniform mat4 MV;
    uniform mat4 P;
    varying vec4 v_color;
    void main() {
        v_color = color;
        gl_PointSize = 2.0;
        gl_Position = P * MV * vec4(vert, 0.0, 1.0);
    }
]]

local fshader = [[
    precision mediump float;
    varying vec4 v_color;
    void main() {
        gl_FragColor = v_color;
    }
]]

local prog = am.program(vshader, fshader);

local view = vbuf:view("vec2")
local cview = cbuf:view("vec4")

local scene = am.use_program(prog)
    ^ am.bind{
        vert = view,
        color = cview,
    }
    ^ am.draw"points"

scene:action(function()
    local dt = am.delta_time
    local mx = math.cos(am.frame_time) * 30
    local my = math.sin(am.frame_time) * 30
    local G = 20
    for i = 0, n - 1 do
        local x = verts[i].x + dt * vels[i].x
        local y = verts[i].y + dt * vels[i].y
        local d = math.sqrt((x-mx)^2 + (y-my)^2)
        local dx = (mx - x) / d
        local dy = (my - y) / d
        local k = d ^ 0.5 * G * dt
        vels[i].x = vels[i].x + dx * k;
        vels[i].y = vels[i].y + dy * k;
        verts[i].x = x
        verts[i].y = y
    end
    vbuf:mark_dirty()
    if win:key_pressed"escape" then
        win:close()
    end
end)

win.scene = am.blend"add" ^ scene
