-- click in the game window to allow it to capture your pointer
-- WASD to walk
-- There is audio - just walk for a bit :)
local win = am.window{
    title = "City",
    width = 1200,
    height = 900,
    resizable = true,
}
win.lock_pointer = true

local create_building_shader
local create_building
local create_floor
local create_kaleidoscope_shader
local create_kaleidoscope_node
local create_blur_shader
local create_blur_node
local init_audio
local chimes
local play_chime

local kaleidoscope_node
local kaleidoscope_shader
local kaleidoscope_fb
local kaleidoscope_tex
local blur_node
local blur_fb
local main_action
local buildings_node
local position_node
local facing_node
local pitch_node
local limit1_node
local building_shader
local root_audio = am.audio_node()


local
function init()
    local buildings_group = am.group()

    buildings_group:append(create_floor(0))

    kaleidoscope_tex = am.texture2d(1024, 1024)
    kaleidoscope_tex.magfilter = "linear"
    kaleidoscope_tex.swrap = "mirrored_repeat"
    kaleidoscope_tex.twrap = "mirrored_repeat"

    kaleidoscope_fb = am.framebuffer(kaleidoscope_tex, true)

    kaleidoscope_shader = create_kaleidoscope_shader()
    building_shader = create_building_shader()
    kaleidoscope_node = create_kaleidoscope_node(kaleidoscope_tex, 1, 1, 1)

    blur_node, blur_fb = create_blur_node()

    position_node = am.translate("MV", vec3(0, -0.1, 0)) ^ buildings_group
    facing_node = am.rotate("MV", quat(0, vec3(0, 1, 0))) ^ position_node
    pitch_node = am.rotate("MV", quat(0, vec3(-1, 0, 0))) ^ facing_node
    limit1_node = 
        am.bind{
            MV = mat4(1),
            P = math.perspective(math.rad(85), 1, 0.01, 100),
            limit1 = 20,
        }
        ^pitch_node
        
    buildings_node = am.use_program(building_shader) ^ limit1_node

    for x = -10, 10, 1 do
        for z = -10, 10, 1 do
            if x ~= 0 or z ~= 0 then
                local height = 2
                local highlight = vec3(0, 0, 0)
                local building = create_building(height, highlight, 0.3, 0.3, vec3(x, 0, z))
                buildings_group:append(building)
            end
        end
    end

    kaleidoscope_fb:render(buildings_node)

    win.scene = blur_node
    win.scene:action(main_action)

    init_audio()
    play_chime(1)

    win.scene:action(am.play(root_audio, true))
end

local zones = {
    {distance = 0,  num_segments = 1,  x_repeat = 1, y_repeat = 1, limit1 = 20,   freq = 110},
    {distance = 2,  num_segments = 2,  x_repeat = 1, y_repeat = 1, limit1 = 3,    freq = 220 * 2^(2/12)},
    {distance = 3,  num_segments = 2,  x_repeat = 2, y_repeat = 1, limit1 = 1,    freq = 220 * 2^(4/12)},
    {distance = 4,  num_segments = 3,  x_repeat = 2, y_repeat = 2, limit1 = 0,    freq = 220 * 2^(6/12)},
    {distance = 5,  num_segments = 4,  x_repeat = 2, y_repeat = 2, limit1 = -5,   freq = 220 * 2^(7/12)},
    {distance = 6,  num_segments = 5,  x_repeat = 2, y_repeat = 3, limit1 = -15,  freq = 220 * 2^(9/12)},
    {distance = 7,  num_segments = 9,  x_repeat = 2, y_repeat = 4, limit1 = -20,  freq = 220 * 2^(11/12)},
    {distance = 8,  num_segments = 17, x_repeat = 2, y_repeat = 2, limit1 = -40,  freq = 220 * 2^(13/12)},
    {distance = 9,  num_segments = 30, x_repeat = 2, y_repeat = 1, limit1 = -80,  freq = 220 * 2^(15/12)},
    {distance = 10, num_segments = 500, x_repeat = 2, y_repeat = 8, limit1 = -500, freq = 220 * 2^(17/12)},
    {distance = 11, num_segments = 1,  x_repeat = 1, y_repeat = 1, limit1 = 20,   freq = 220},
}

local current_zone = 1

local
function update_kaleidoscope_node()
    local zn = #zones
    local position = position_node.position
    local d = math.max(math.abs(position.x), math.abs(position.z))
    for i, zone in ipairs(zones) do
        if d < zone.distance then
            zn = i - 1
            break
        end
    end
    if zn ~= current_zone then
        current_zone = zn
        local zone = zones[zn]
        kaleidoscope_node = create_kaleidoscope_node(kaleidoscope_tex, zone.num_segments, zone.x_repeat, zone.y_repeat)
        limit1_node.limit1 = zone.limit1
        play_chime(zn)
    end
end

local walk_t = 0

local facing = 0
function main_action()
    local turn_speed = 2
    local walk_speed = 0.3
    local strafe_speed = 0.2
    local pos_xz = position_node.position.xz
    local pos_y = position_node.position.y
    if win:key_down("w") or win:mouse_down("left") then
        local dir = vec2(math.cos(facing+math.pi/2), math.sin(facing+math.pi/2))
        pos_xz = pos_xz + dir * walk_speed * am.delta_time
    elseif win:key_down("s") then
        local dir = vec2(math.cos(facing+math.pi/2), math.sin(facing+math.pi/2))
        pos_xz = pos_xz - dir * walk_speed * am.delta_time
    end
    if win:key_down("a") then
        local dir = vec2(math.cos(facing+math.pi), math.sin(facing+math.pi))
        pos_xz = pos_xz - dir * strafe_speed * am.delta_time
    elseif win:key_down("d") then
        local dir = vec2(math.cos(facing+math.pi), math.sin(facing+math.pi))
        pos_xz = pos_xz + dir * strafe_speed * am.delta_time
    end
    if win:key_pressed("escape") then
        win:close()
    end
    if win:key_down("w") or win:key_down("s") or win:mouse_down("left") then
        walk_t = walk_t + am.delta_time
        pos_y = -0.1 + math.sin(walk_t*10) * 0.005
    end
    position_node.position = vec3(pos_xz.x, pos_y, pos_xz.y)

    update_kaleidoscope_node()

    local mouse_pos = win:mouse_norm_position()
    facing = mouse_pos.x * 1.5
    facing_node.rotation = quat(facing, vec3(0, 1, 0))
    pitch_node.rotation = quat(mouse_pos.y * 1.5 - 1, vec3(-1, 0, 0))

    kaleidoscope_fb:clear(true, true)
    kaleidoscope_fb:render(buildings_node)
    blur_fb:render(kaleidoscope_node.node)

    --local stats = am.perf_stats()
    --log("FPS: %0.2f [%0.2f]", stats.avg_fps, stats.min_fps)
end

function create_building(height, highlight, width, depth, position)
    local num_verts = 4 * 2 * 3 -- 4 sides, 2 triangles/side, 3 vertices per triangle
    local buffer = am.buffer(num_verts * 3 * 4 * 2) -- 3 floats/vert, 4 bytes per float, x2 for color components
    local vert_view = buffer:view("vec3", 0, 24)
    local color_view = buffer:view("vec3", 12, 24)

    local front_color = vec3(0.4) + highlight
    local left_color = vec3(0.6) + highlight
    local back_color = vec3(0.8) + highlight
    local right_color = vec3(0.3) + highlight

    local l = position.x - width/2
    local r = position.x + width/2
    local b = position.y
    local t = position.y + height
    local n = position.z + depth/2
    local f = position.z - depth/2
    
    -- front
    vert_view[1] = vec3(l, b, n)
    vert_view[2] = vec3(r, t, n)
    vert_view[3] = vec3(l, t, n)
    vert_view[4] = vec3(l, b, n)
    vert_view[5] = vec3(r, b, n)
    vert_view[6] = vec3(r, t, n)
    for i = 1, 6 do
        color_view[i] = front_color
    end

    -- left
    vert_view[7]  = vec3(l, b, f)
    vert_view[8]  = vec3(l, t, n)
    vert_view[9]  = vec3(l, t, f)
    vert_view[10] = vec3(l, b, f)
    vert_view[11] = vec3(l, b, n)
    vert_view[12] = vec3(l, t, n)
    for i = 7, 12 do
        color_view[i] = left_color
    end

    -- right
    vert_view[13] = vec3(r, b, f)
    vert_view[14] = vec3(r, t, f)
    vert_view[15] = vec3(r, t, n)
    vert_view[16] = vec3(r, b, f)
    vert_view[17] = vec3(r, t, n)
    vert_view[18] = vec3(r, b, n)
    for i = 13, 18 do
        color_view[i] = right_color
    end

    -- back
    vert_view[19] = vec3(l, b, f)
    vert_view[20] = vec3(l, t, f)
    vert_view[21] = vec3(r, t, f)
    vert_view[22] = vec3(l, b, f)
    vert_view[23] = vec3(r, t, f)
    vert_view[24] = vec3(r, b, f)
    for i = 19, 24 do
        color_view[i] = back_color
    end

    return 
        am.bind{
            vert = vert_view,
            color = color_view,
        }
        ^ am.draw("triangles")
end

function create_floor(height)
    local num_verts = 6
    local buffer = am.buffer(num_verts * 3 * 4 * 2) -- 3 floats/vert, 4 bytes per float, x2 for color components
    local vert_view = buffer:view("vec3", 0, 24)
    local color_view = buffer:view("vec3", 12, 24)

    local d = 100
    local l = -d
    local r = d
    local n = d
    local f = -d
    
    local color = vec3(0.05)

    vert_view[1] = vec3(l, height, n)
    vert_view[2] = vec3(r, height, n)
    vert_view[3] = vec3(r, height, f)
    vert_view[4] = vec3(l, height, n)
    vert_view[5] = vec3(r, height, f)
    vert_view[6] = vec3(l, height, f)
    for i = 1, 6 do
        color_view[i] = color
    end

    return 
        am.bind{
            vert = vert_view,
            color = color_view,
        }
        ^ am.draw("triangles")
end

function create_kaleidoscope_node(texture, num_segments, x_repeat, y_repeat)
    local vbuf = am.buffer(16 * 3 * (num_segments == 1 and 2 or num_segments == 2 and 4 or num_segments))
    local verts = vbuf:view("vec2", 0, 16)
    local uvs = vbuf:view("vec2", 8, 16)
    local uv_scale = 2
    local r = 3
    if num_segments == 1 then
        verts[1] = vec2(-1, -1)
        verts[2] = vec2(1, -1)
        verts[3] = vec2(1, 1)
        verts[4] = vec2(-1, -1)
        verts[5] = vec2(1, 1)
        verts[6] = vec2(-1, 1)
        uvs[1] = vec2(0, 0)
        uvs[2] = vec2(x_repeat, 0)
        uvs[3] = vec2(x_repeat, y_repeat)
        uvs[4] = vec2(0, 0)
        uvs[5] = vec2(x_repeat, y_repeat)
        uvs[6] = vec2(0, y_repeat)
    elseif num_segments == 2 then
        verts[1] = vec2(-1, 0)
        verts[2] = vec2(1, 0)
        verts[3] = vec2(1, 1)
        verts[4] = vec2(-1, 0)
        verts[5] = vec2(1, 1)
        verts[6] = vec2(-1, 1)
        uvs[1] = vec2(0, 0)
        uvs[2] = vec2(x_repeat, 0)
        uvs[3] = vec2(x_repeat, y_repeat)
        uvs[4] = vec2(0, 0)
        uvs[5] = vec2(x_repeat, y_repeat)
        uvs[6] = vec2(0, y_repeat)
        verts[7] = vec2(-1, -1)
        verts[8] = vec2(1, -1)
        verts[9] = vec2(1, 0)
        verts[10] = vec2(-1, -1)
        verts[11] = vec2(1, 0)
        verts[12] = vec2(-1, 0)
        uvs[7] = vec2(0, y_repeat)
        uvs[8] = vec2(x_repeat, y_repeat)
        uvs[9] = vec2(x_repeat, 0)
        uvs[10] = vec2(0, y_repeat)
        uvs[11] = vec2(x_repeat, 0)
        uvs[12] = vec2(0, 0)
    else
        for i = 0, num_segments-1 do
            local angle1 = (i / num_segments) * math.pi * 2
            local angle2 = ((i + 1) / num_segments) * math.pi * 2
            local d = angle2 - angle1
            verts[i*3+1] = vec2(0, 0)
            verts[i*3+2] = vec2(math.cos(angle2)*r, math.sin(angle2)*r)
            verts[i*3+3] = vec2(math.cos(angle1)*r, math.sin(angle1)*r)
            uvs[i*3+1] = vec2(0, 0)
            uvs[i*3+2] = vec2(0-x_repeat, y_repeat)
            uvs[i*3+3] = vec2(0+x_repeat, y_repeat)
        end
    end

    local rotation_node = 
        am.rotate("MVP", quat(0))
        ^am.bind{
            tex = texture,
            uv = uvs,
            vert = verts,
        }
        ^am.draw("triangles")
    local node = 
        am.use_program(kaleidoscope_shader)
        ^am.bind{MVP = mat4(1)}
        ^rotation_node

    return {
        node = node,
        rotation_node = rotation_node
    }
end

function create_blur_node() 
    local pptexture = am.texture2d(1024)
    pptexture.magfilter = "linear"
    local ppfb = am.framebuffer(pptexture)
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
        am.use_program(create_blur_shader())
        ^am.bind{
            tex = pptexture,
            uv = uvs2,
            vert = verts2,
        }
        ^am.draw("triangles")
    return node2, ppfb, pptexture
end

function create_building_shader() 
    local vshader = [[
        precision mediump float;
        uniform mat4 MV;
        uniform mat4 P;
        attribute vec3 vert;
        attribute vec3 color;
        varying vec3 v_color;
        varying vec3 v_pos;
        void main() {
            v_color = color;
            v_pos = vert;
            gl_Position = P * MV * vec4(vert, 1);
        }
    ]]
    local fshader = [[
        #define limit2 30.0
        precision mediump float;
        uniform float limit1;
        varying vec3 v_color;
        varying vec3 v_pos;
        void main() {
            vec3 color = v_color * pow(clamp(gl_FragCoord.w, 0.0, 1.0), 1.4);
            if (gl_FragCoord.w > limit1) {
                vec3 noise = vec3(
                    fract(sqrt(gl_FragCoord.w * gl_FragCoord.x)) * abs(sin(v_pos.x * 13.0)),
                    fract(sqrt(gl_FragCoord.w * gl_FragCoord.y)) * abs(sin(v_pos.z * 10.0)),
                    fract(sqrt(gl_FragCoord.w * gl_FragCoord.y)) * abs(cos(v_pos.x * 7.0))
                );
                gl_FragColor = vec4(mix(color, noise, min(1.0, (gl_FragCoord.w - limit1) / (limit2 - limit1))), 1.0);
            } else {
                gl_FragColor = vec4(color, 1);
            }
        }
    ]]
    return am.program(vshader, fshader)
end

function create_kaleidoscope_shader()
    local vshader = [[
        precision mediump float;
        attribute vec2 vert;
        attribute vec2 uv;
        uniform mat4 MVP;
        varying vec2 v_uv;
        void main() {
            v_uv = uv;
            gl_Position = MVP * vec4(vert, 0, 1);
        }
    ]]
    local fshader = [[
        precision mediump float;
        varying vec2 v_uv;
        uniform sampler2D tex;
        void main() {
            gl_FragColor = texture2D(tex, v_uv);
        }
    ]]
    return am.program(vshader, fshader)
end

function create_blur_shader()
    local vshader = [[
        precision mediump float;
        attribute vec2 vert;
        attribute vec2 uv;
        varying vec2 v_uv;
        varying vec2 v_pos;
        void main() {
            v_pos = vert;
            v_uv = uv;
            gl_Position = vec4(vert, 0, 1);
        }
    ]]
    local fshader = [[
        #define n 2
        precision mediump float;
        varying vec2 v_uv;
        varying vec2 v_pos;
        uniform sampler2D tex;
        void main() {
            float e = pow(length(v_pos), 2.0) / 300.0;
            vec4 sum = vec4(0.0);
            for (int i = -n; i <= n; i++) {
                for (int j = -n; j <= n; j++) {
                    vec2 d = vec2(float(i) * e, float(j) * e);
                    vec4 samp = texture2D(tex, v_uv + d);
                    sum += samp;
                }
            }
            gl_FragColor = sum / pow(float(n) * 2.0 + 1.0, 2.0);
                //+ vec4(e * 50.0, 0.0, 0.0, 0.0);
        }
    ]]
    return am.program(vshader, fshader)
end

local num_active_chimes = 1

function init_audio()
    chimes = {}
    for i = 1, #zones do
        local rate = 44100
        local freq = zones[i].freq
        local num_samples = math.floor(rate / freq)
        local audio_buf = am.buffer(4 * 2 * num_samples)
        local left_channel = audio_buf:view("float", 0, 4)
        local right_channel = audio_buf:view("float", num_samples*4, 4)
        for i = 0, num_samples-1 do
            local sample = math.sin((i/rate) * 2 * math.pi * freq)
            left_channel[i+1] = sample 
            right_channel[i+1] = sample
        end
        chimes[i] = am.track(am.audio_buffer(audio_buf, 2, rate), true, 1)
        chimes[i].playback_speed = 1
        chimes[i] = chimes[i]:gain(0.1)
    end
    local primes = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73 }
    win.scene:action(function()
        local pos = win:mouse_norm_position()
        for i = 1, num_active_chimes do
            local z = math.cos(pos.x * pos.y + i) * 0.4 + 0.6
            chimes[i].value = (math.sin(am.frame_time * primes[i]) * 0.4 + 0.6) * math.min(0.4, 1/num_active_chimes)
                * z
        end
    end)
end

function play_chime(c)
    root_audio:remove_all()
    if c < #zones and c > 1 then
        for i = c, 1, -1 do
            root_audio:add(chimes[i])
        end
        num_active_chimes = c
    else
        num_active_chimes = 0
    end
end

init()
