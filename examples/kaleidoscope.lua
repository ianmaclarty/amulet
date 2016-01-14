local win = am.window{width = 400, height = 400}

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
local prog1 = am.program(vshader1, fshader1)

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
local prog2 = am.program(vshader2, fshader2)

-- setup vertices and texture coords
local num_segments = 9
local vbuf = am.buffer(16 * 3 * num_segments)
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
local pattern_tex_buf = am.buffer(pattern_tex_size^2*4)
local pattern_tex_view = pattern_tex_buf:view("uint", 0, 4)
for i = 1, pattern_tex_size^2 do
    pattern_tex_view[i] = 255 * 2^24 + math.random(2^24)
end
local pattern_img_buf = am.image_buffer(pattern_tex_buf, pattern_tex_size, pattern_tex_size)
local pattern_texture = am.texture2d(pattern_img_buf)
pattern_texture.wrap = "mirrored_repeat"

-- create node that will render the kaleidoscope vertices using
-- the program and texture we created earlier
local t_node = 
    am.bind{
        vert = verts,
        uv = uvs,
        tex = pattern_texture,
        t = 0,
    }
    ^am.draw("triangles")
local rotation_node = am.rotate("MVP", quat(0)) ^ t_node
local node1 = 
    am.use_program(prog1)
    ^am.bind{MVP = mat4(1)}
    ^rotation_node

-- create texture for post-processing (applying blur)
local pptexture = am.texture2d(512)
pptexture.filter = "linear"

-- create framebuffer so we can draw into pptexture
local ppfb = am.framebuffer(pptexture)

-- create node to apply post-processing
local buf2 = am.buffer(4 * 4 * 6)
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
local node2 = 
    am.use_program(prog2)
    ^am.bind{
        vert = verts2,
        uv = uvs2,
        tex = pptexture,
        MVP = mat4(1),
    }
    ^am.draw("triangles")

-- set the post processing node as the scene
win.scene = node2

-- create an action on the scene that updates the various uniforms
-- and then renders the kaleidoscope into the post-processing framebuffer
-- which then gets automatically drawn to the window using the
-- blur shader (since it's the scene).
win.scene:action(function()
    pattern_tex_view[math.random(pattern_tex_size^2)] = 255 * 2^24 + math.random(2^24)
    rotation_node.rotation = quat(am.frame_time)
    t_node.t = am.frame_time
    ppfb:render(node1)
    if win:key_pressed("escape") then
        win:close()
    end
end)
