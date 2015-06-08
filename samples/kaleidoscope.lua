local win = amulet.window({})

-- create the shader program for rendering kaleidoscope
local vshader1 = [[
    precision mediump float;
    attribute vec2 vert;
    attribute vec2 uv;
    uniform float t;
    uniform mat4 MVP;
    varying vec2 v_uv;
    void main() {
        v_uv = (MVP * vec4(uv, 0.0, 1.0)).xy + vec2(sin(t*0.7), cos(t*0.3));
        gl_Position = MVP * vec4(vert, 0, 1);
    }
]]
local fshader1 = [[
    precision mediump float;
    varying vec2 v_uv;
    uniform sampler2D tex;
    void main() {
        gl_FragColor = texture2D(tex, v_uv);
    }
]]
local prog1 = amulet.program(vshader1, fshader1)

-- create the shader program for adding blur
local vshader2 = [[
    precision mediump float;
    attribute vec2 vert;
    attribute vec2 uv;
    uniform float t;
    uniform mat4 MVP;
    varying vec2 v_uv;
    varying vec2 v_pos;
    void main() {
        v_pos = vert;
        v_uv = uv;
        gl_Position = MVP * vec4(vert, 0, 1);
    }
]]
local fshader2 = [[
    #define n 2
    precision mediump float;
    varying vec2 v_uv;
    varying vec2 v_pos;
    uniform sampler2D tex;
    void main() {
        float e = pow(length(v_pos), 2.0) / 100.0;
        vec4 sum = vec4(0.0);
        for (int i = -n; i <= n; i++) {
            for (int j = -n; j <= n; j++) {
                vec2 d = vec2(float(i) * e, float(j) * e);
                vec4 samp = texture2D(tex, v_uv + d);
                sum += samp;
            }
        }
        gl_FragColor = sum / pow(float(n) * 2.0 + 1.0, 2.0)
            + vec4(e * 50.0, 0.0, 0.0, 0.0);
    }
]]
local prog2 = amulet.program(vshader2, fshader2)

-- setup vertices and texture coords
local num_segments = 9
local vbuf = amulet.buffer(16 * 3 * num_segments)
local verts = vbuf:view("vec2", 0, 16)
local uvs = vbuf:view("vec2", 8, 16)
local uv_scale = 5
for i = 0, num_segments-1 do
    local angle1 = (i / num_segments) * math.pi * 2
    local angle2 = ((i + 1) / num_segments) * math.pi * 2
    verts[i*3+1] = vec2(0, 0)
    verts[i*3+2] = vec2(math.cos(angle1)*2, math.sin(angle1)*2)
    verts[i*3+3] = vec2(math.cos(angle2)*2, math.sin(angle2)*2)
    uvs[i*3+1] = vec2(0, 0)
    uvs[i*3+2] = vec2(0, uv_scale)
    uvs[i*3+3] = vec2(uv_scale, 0)
end

-- create pattern texture
local pattern_tex_size = 8
local pattern_tex_buf = amulet.buffer(pattern_tex_size^2*2)
local pattern_tex_view = pattern_tex_buf:view("ushort", 0, 2)
for i = 1, pattern_tex_size^2 do
    pattern_tex_view[i] = math.random(2^16)
end
local pattern_texture = amulet.texture2d{
    buffer = pattern_tex_buf,
    width = pattern_tex_size,
    height = pattern_tex_size,
    swrap = "mirrored_repeat",
    twrap = "mirrored_repeat",
    format = "rgb",
    type = "565"
}

-- create node that will render the kaleidoscope vertices using
-- the program and texture we created earlier
local t_node = amulet.draw_arrays()
    :bind_array("vert", verts)
    :bind_array("uv", uvs)
    :bind_sampler2d("tex", pattern_texture)
    :bind_float("t", 0)
local rotation_node = t_node:rotate("MVP")
local node1 = rotation_node
    :bind_mat4("MVP", mat4(1))
    :bind_program(prog1)

-- create texture for post-processing (applying blur)
local pptexture = amulet.texture2d{width = 512, height = 512, magfilter = "linear"}

-- create framebuffer so we can draw into pptexture
local ppfb = amulet.framebuffer(pptexture)

-- create node to apply post-processing
local buf2 = amulet.buffer(4 * 4 * 6)
local verts2 = buf2:view("vec2", 0, 16)
local uvs2 = buf2:view("vec2", 8, 16)
verts2[1] = vec2(-1, -1)
verts2[2] = vec2(-1, 1)
verts2[3] = vec2(1, 1)
verts2[4] = vec2(-1, -1)
verts2[5] = vec2(1, 1)
verts2[6] = vec2(1, -1)
uvs2[1] = vec2(0, 0)
uvs2[2] = vec2(0, 1)
uvs2[3] = vec2(1, 1)
uvs2[4] = vec2(0, 0)
uvs2[5] = vec2(1, 1)
uvs2[6] = vec2(1, 0)
local node2 = amulet.draw_arrays()
    :bind_array("vert", verts2)
    :bind_array("uv", uvs2)
    :bind_sampler2d("tex", pptexture)
    :bind_mat4("MVP", mat4(1))
    :bind_program(prog2)

-- set the post processing node as the root
win.root = node2

-- create an action on the root that updates the various uniforms
-- and then renders the kaleidoscope into the post-processing framebuffer
-- which then gets automatically drawn to the window using the
-- blur shader (since it's the root).
win.root:action(function()
    pattern_tex_view[math.random(pattern_tex_size^2)] = math.random(2^16)
    rotation_node.angle = amulet.frame_time
    t_node.value = amulet.frame_time
    ppfb:render(node1)
    return 0
end)
