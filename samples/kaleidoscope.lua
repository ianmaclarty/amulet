local win = amulet.create_window({})

-- create the shader program
local vshader = [[
    precision mediump float;
    attribute vec2 vert;
    attribute vec2 uv;
    uniform float t;
    uniform mat4 MVP;
    varying vec2 v_uv;
    void main() {
        v_uv = uv + vec2(t, t);
        gl_Position = MVP * vec4(vert, 0, 1);
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
local texture = amulet.create_texture2d{
    buffer = tbuf,
    width = n,
    height = n,
    swrap = "mirrored_repeat",
    twrap = "mirrored_repeat",
    format = "rgb",
    type = "565"
}

-- create node that will render the vertices using
-- the program and texture we created
local node = amulet.draw_arrays()
    :bind_array("vert", verts)
    :bind_array("uv", uvs)
    :bind_sampler2d("tex1", texture)
    :bind_float("t", 0):alias("t")
    :rotate2d("MVP"):alias("rotation")
    :bind_mat4("MVP", math.mat4(1))
    :program(prog)

-- create an action on the node that changes the colour
-- of random pixels in the texture, rotates the MVP matrix
-- and updates the t uniform
node:action(function()
    tview[math.random(n^2)] = math.random(2^16)
    node.rotation.angle = amulet.frame_time()
    node.t.value = amulet.frame_time()
    return 0
end)

-- set the node as the root of the window
win.root = node
