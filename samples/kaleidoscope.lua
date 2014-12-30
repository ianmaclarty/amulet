local win = amulet.create_window({})

-- create the shader program
local vshader = [[
    precision mediump float;
    attribute vec2 vert;
    attribute vec2 uv;
    varying vec2 v_uv;
    uniform float t;
    void main() {
        v_uv = uv + vec2(t, t);
        float len = length(vert);
        vec2 pos = vert;
        if (len > 0.1) {
            float angle = atan(vert.y, vert.x) + t;
            pos = vec2(cos(angle), sin(angle)) * len;
        }
        gl_Position = vec4(pos, 0, 1);
    }
]]
local fshader = [[
    precision mediump float;
    varying vec2 v_uv;
    uniform sampler2D tex1;
    void main() {
        gl_FragColor = texture2D(tex1, v_uv);
    }
]]
local prog = amulet.program(vshader, fshader)

-- setup vertices and texture coords
local num_segments = 9
local vbuf = amulet.buffer(16 * 3 * num_segments)
local verts = vbuf:view("float2", 0, 16)
local uvs = vbuf:view("float2", 8, 16)
local uv_scale = 5
for i = 0, num_segments-1 do
    local angle1 = (i / num_segments) * math.pi * 2
    local angle2 = ((i + 1) / num_segments) * math.pi * 2
    verts[i*3+1] = math.vec2(0, 0)
    verts[i*3+2] = math.vec2(math.cos(angle1)*2, math.sin(angle1)*2)
    verts[i*3+3] = math.vec2(math.cos(angle2)*2, math.sin(angle2)*2)
    uvs[i*3+1] = math.vec2(0, 0)
    uvs[i*3+2] = math.vec2(0, uv_scale)
    uvs[i*3+3] = math.vec2(uv_scale, 0)
end

-- create texture
local n = 8
local tbuf = amulet.buffer(n^2*2)
local tview = tbuf:view("ushort", 0, 2)
for i = 1, n^2 do
    tview[i] = math.random(2^16)
end
tbuf:setup_texture2d(n, n,
    "nearest", "nearest",
    "mirrored_repeat", "mirrored_repeat",
    "rgb", "565")

-- create node that will render the vertices using
-- the program and texture we created
local node = amulet.draw_arrays()
    :bind_array("vert", verts)
    :bind_array("uv", uvs)
    :bind_sampler2d("tex1", tbuf)
    :bind_float("t", 0):alias("t")
    :program(prog)

-- create an action on the node that changes the colour
-- of random pixels in the texture and updates the t uniform
node:action(function()
    tview[math.random(n^2)] = math.random(2^16)
    node.t.value = amulet.frame_time()
    return 0
end)

-- set the node as the root of the window
win.root = node
