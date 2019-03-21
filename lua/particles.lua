local particles2d_shader

local
function get_particles2d_shader()
    if particles2d_shader then
        return particles2d_shader
    end
    local v = [[
        precision highp float;
        uniform mat4 P;
        uniform mat4 MV;
        attribute vec3 vert;
        attribute vec4 color;
        attribute vec2 offset;
        varying vec4 v_color;
        void main() {
            v_color = color;
            gl_Position = P * MV * vec4(vert.xy + offset * vert.z, 0.0, 1.0);
        }
    ]]
    local f = [[
        precision mediump float;
        varying vec4 v_color;
        void main() {
            gl_FragColor = clamp(v_color, 0.0, 1.0);
        }
    ]]
    particles2d_shader = am.program(v, f)
    return particles2d_shader
end

local particles2d_shader_tex

local
function get_particles2d_shader_tex()
    if particles2d_shader_tex then
        return particles2d_shader_tex
    end
    local v = [[
        precision highp float;
        uniform mat4 P;
        uniform mat4 MV;
        uniform vec2 uv_offset;
        uniform vec2 uv_scale;
        attribute vec3 vert;
        attribute vec4 color;
        attribute vec2 offset;
        varying vec4 v_color;
        varying vec2 v_uv;
        void main() {
            v_color = color;
            v_uv = uv_scale * (offset * 0.5 + 0.5) + uv_offset;
            gl_Position = P * MV * vec4(vert.xy + offset * vert.z, 0.0, 1.0);
        }
    ]]
    local f = [[
        precision mediump float;
        uniform sampler2D tex;
        varying vec4 v_color;
        varying vec2 v_uv;
        void main() {
            gl_FragColor = clamp(texture2D(tex, v_uv) * v_color, 0.0, 1.0);
        }
    ]]
    particles2d_shader_tex = am.program(v, f)
    return particles2d_shader_tex
end

-- settings:
--      source_pos
--      source_pos_var
--      start_size
--      start_size_var
--      end_size
--      end_size_var
--      angle
--      angle_var
--      speed
--      speed_var
--      life
--      life_var
--      start_color
--      start_color_var
--      end_color
--      end_color_var
--      emission_rate
--      start_particles
--      max_particles
--      gravity
--      sprite_source
--      warmup_time
--      damping
function am.particles2d(opts)
    local max_particles = opts.max_particles or 100
    local start_particles = opts.start_particles or 0
    if start_particles > max_particles then
        error("start_particles is larger than max_particles", 2)
    end
    local source_pos_x = opts.source_pos and opts.source_pos.x or 0
    local source_pos_y = opts.source_pos and opts.source_pos.y or 0
    local source_pos_x_var = opts.source_pos_var and opts.source_pos_var.x or 0
    local source_pos_y_var = opts.source_pos_var and opts.source_pos_var.y or 0
    local start_size = (opts.start_size or 20) / 2
    local start_size_var = (opts.start_size_var or 0) / 2
    local size_changes = not not opts.end_size
    local end_size = opts.end_size and opts.end_size / 2
    local end_size_var = (opts.end_size_var or 0) / 2
    local angle = opts.angle or 0
    local angle_var = opts.angle_var or 0
    local speed = opts.speed or 100
    local speed_var = opts.speed_var or 0
    local life = opts.life or 1
    local life_var = opts.life_var or 0
    if life_var >= life then
        life_var = life - 0.001 -- prevent zero time-to-live
    end
    local start_r = opts.start_color and opts.start_color.r or 1
    local start_g = opts.start_color and opts.start_color.g or 1
    local start_b = opts.start_color and opts.start_color.b or 1
    local start_a = opts.start_color and opts.start_color.a or 1
    local start_r_var = opts.start_color_var and opts.start_color_var.r or 0
    local start_g_var = opts.start_color_var and opts.start_color_var.g or 0
    local start_b_var = opts.start_color_var and opts.start_color_var.b or 0
    local start_a_var = opts.start_color_var and opts.start_color_var.a or 0
    local color_changes = not not opts.end_color
    local end_r = opts.end_color and opts.end_color.r
    local end_g = opts.end_color and opts.end_color.g
    local end_b = opts.end_color and opts.end_color.b
    local end_a = opts.end_color and opts.end_color.a
    local end_r_var = opts.end_color_var and opts.end_color_var.r or 0
    local end_g_var = opts.end_color_var and opts.end_color_var.g or 0
    local end_b_var = opts.end_color_var and opts.end_color_var.b or 0
    local end_a_var = opts.end_color_var and opts.end_color_var.a or 0
    local emission_rate = opts.emission_rate or 50
    local gravity_x = opts.gravity and opts.gravity.x or 0
    local gravity_y = opts.gravity and opts.gravity.y or 0
    local gravity_x_2 = gravity_x / 2
    local gravity_y_2 = gravity_y / 2
    local damping = opts.damping or 0
    local sprite
    if opts.sprite_source then
        sprite = am._convert_sprite_source(opts.sprite_source)
    end
    local warmup_time = opts.warmup_time or 0

    local n = 0  -- num active particles
    local emit_counter = 0

    local x = {}
    local y = {}
    local z = {} -- size
    local dz = {}
    local r = {}
    local g = {}
    local b = {}
    local a = {}
    local dr = {}
    local dg = {}
    local db = {}
    local da = {}
    local time_to_live = {}
    local speed_x = {}
    local speed_y = {}

    local i = 0
    local k = 0
    local indices = {}
    for j = 1, max_particles do
        indices[i + 1] = k + 1
        indices[i + 2] = k + 2
        indices[i + 3] = k + 3
        indices[i + 4] = k + 1
        indices[i + 5] = k + 3
        indices[i + 6] = k + 4
        i = i + 6
        k = k + 4
    end

    local elemsview
    if max_particles * 4 < 2 ^ 16 then
        elemsview = am.ushort_elem_array(indices)
    else
        elemsview = am.uint_elem_array(indices)
    end

    local offsets = {}
    i = 0
    for j = 1, max_particles do
        offsets[i + 1] = vec2(-1, 1)
        offsets[i + 2] = vec2(-1, -1)
        offsets[i + 3] = vec2(1, -1)
        offsets[i + 4] = vec2(1, 1)
        i = i + 4
    end

    local offsetview = am.vec2_array(offsets)

    local uv_offset
    local uv_scale
    if sprite then
        uv_offset = vec2(sprite.s1, sprite.t1)
        uv_scale = vec2(sprite.s2 - sprite.s1, sprite.t2 - sprite.t1)
    end

    local num_verts = max_particles * 4
    local stride = 12 + 16
    local vertbuf = am.buffer(num_verts * stride)
    vertbuf.usage = "dynamic"
    local vertview = vertbuf:view("vec3", 0, stride)
    local x1_view  = vertbuf:view("float", 0, stride * 4)
    local y1_view  = vertbuf:view("float", 4, stride * 4)
    local z1_view = vertbuf:view("float", 8, stride * 4) -- z stores size
    local xyz1_view = vertbuf:view("vec3", 0,  stride * 4)
    local xyz2_view = vertbuf:view("vec3", 1*stride,  stride * 4)
    local xyz3_view = vertbuf:view("vec3", 2*stride, stride * 4)
    local xyz4_view = vertbuf:view("vec3", 3*stride, stride * 4)
    local r1_view = vertbuf:view("float", 12, stride * 4)
    local g1_view = vertbuf:view("float", 16, stride * 4)
    local b1_view = vertbuf:view("float", 20, stride * 4)
    local a1_view = vertbuf:view("float", 24, stride * 4)
    local rgba1_view = vertbuf:view("vec4", 12, stride * 4)
    local rgba2_view = vertbuf:view("vec4", 12+stride*1, stride * 4)
    local rgba3_view = vertbuf:view("vec4", 12+stride*2, stride * 4)
    local rgba4_view = vertbuf:view("vec4", 12+stride*3, stride * 4)
    local colorview = vertbuf:view("vec4", 12, stride)

    local shader
    local node
    if sprite then
        shader = get_particles2d_shader_tex()
    else
        shader = get_particles2d_shader()
    end

    local node = am.use_program(shader)
        ^ am.bind{
            vert = vertview,
            offset = offsetview,
            color = colorview,
            tex = sprite and sprite.texture or nil,
            uv_offset = uv_offset,
            uv_scale = uv_scale,
        }
        ^ am.draw("triangles", elemsview)

    local
    function set_verts()
        x1_view:set(x, 1, n)
        y1_view:set(y, 1, n)
        z1_view:set(z, 1, n)
        xyz2_view:set(xyz1_view, 1, n)
        xyz3_view:set(xyz1_view, 1, n)
        xyz4_view:set(xyz1_view, 1, n)
        r1_view:set(r, 1, n)
        g1_view:set(g, 1, n)
        b1_view:set(b, 1, n)
        a1_view:set(a, 1, n)
        rgba2_view:set(rgba1_view, 1, n)
        rgba3_view:set(rgba1_view, 1, n)
        rgba4_view:set(rgba1_view, 1, n)
        node"draw".count = n * 6
    end

    local rnd = math.random
    local cos = math.cos
    local sin = math.sin

    local
    function add_particle()
        n = n + 1
        time_to_live[n] = life + (rnd() * 2 - 1) * life_var
        x[n] = source_pos_x + (rnd() * 2 - 1) * source_pos_x_var
        y[n] = source_pos_y + (rnd() * 2 - 1) * source_pos_y_var
        z[n] = start_size + (rnd() * 2 - 1) * start_size_var
        if size_changes then
            dz[n] = (end_size + (rnd() * 2 - 1) * end_size_var - z[n]) / time_to_live[n]
        else
            dz[n] = 0
        end
        r[n] = start_r + (rnd() * 2 - 1) * start_r_var
        g[n] = start_g + (rnd() * 2 - 1) * start_g_var
        b[n] = start_b + (rnd() * 2 - 1) * start_b_var
        a[n] = start_a + (rnd() * 2 - 1) * start_a_var
        if color_changes then
            dr[n] = (end_r + (rnd() * 2 - 1) * end_r_var - r[n]) / time_to_live[n]
            dg[n] = (end_g + (rnd() * 2 - 1) * end_g_var - g[n]) / time_to_live[n]
            db[n] = (end_b + (rnd() * 2 - 1) * end_b_var - b[n]) / time_to_live[n]
            da[n] = (end_a + (rnd() * 2 - 1) * end_a_var - a[n]) / time_to_live[n]
        else
            dr[n] = 0
            dg[n] = 0
            db[n] = 0
            da[n] = 0
        end
        local speed = speed + (rnd() * 2 - 1) * speed_var
        local angle = angle + (rnd() * 2 - 1) * angle_var
        speed_x[n] = cos(angle) * speed
        speed_y[n] = sin(angle) * speed
    end

    local
    function update(dt)
        -- update existing particles
        local i = 1
        local damp_factor = (1 - damping * dt)
        while i <= n do
            time_to_live[i] = time_to_live[i] - dt
            if time_to_live[i] >= 0 then
                x[i] = x[i] + dt * (speed_x[i] + dt * gravity_x_2)
                y[i] = y[i] + dt * (speed_y[i] + dt * gravity_y_2)
                speed_x[i] = (speed_x[i] + gravity_x * dt) * damp_factor
                speed_y[i] = (speed_y[i] + gravity_y * dt) * damp_factor
                z[i] = z[i] + dz[i] * dt
                r[i] = r[i] + dr[i] * dt
                g[i] = g[i] + dg[i] * dt
                b[i] = b[i] + db[i] * dt
                a[i] = a[i] + da[i] * dt
                i = i + 1
            else
                if i ~= n then
                    x[i] = x[n];
                    y[i] = y[n];
                    z[i] = z[n];
                    dz[i] = dz[n];
                    r[i] = r[n];
                    g[i] = g[n];
                    b[i] = b[n];
                    a[i] = a[n];
                    dr[i] = dr[n];
                    dg[i] = dg[n];
                    db[i] = db[n];
                    da[i] = da[n];
                    time_to_live[i] = time_to_live[n];
                    speed_x[i] = speed_x[n];
                    speed_y[i] = speed_y[n];
                end
                n = n - 1
            end
        end

        -- generate new particles
        if emission_rate > 0 then
            local delay = 1 / emission_rate
            emit_counter = emit_counter + dt
            while n < max_particles and emit_counter >= delay do
                add_particle()
                emit_counter = emit_counter - delay
            end
        end

        set_verts()
    end
    
    node:action(function() update(am.delta_time) end)

    local
    function init()
        n = 0
        for i = 1, start_particles do
            add_particle()
        end
        set_verts()

        local dt = 1/60
        warmup_time = opts.warmup_time or 0
        while warmup_time > 0 do
            update(dt)
            warmup_time = warmup_time - dt
        end
    end

    init()

    function node:get_source_pos()
        return vec2(source_pos_x, source_pos_y)
    end
    function node:set_source_pos(p)
        source_pos_x = p.x
        source_pos_y = p.y
    end
    function node:get_source_pos_var()
        return vec2(source_pos_x_var, source_pos_y_var)
    end
    function node:set_source_pos_var(v)
        source_pos_x_var = v.x
        source_pos_y_var = v.y
    end
    function node:get_start_size()
        return start_size
    end
    function node:set_start_size(v)
        start_size = v
    end
    function node:get_start_size_var()
        return start_size_var
    end
    function node:set_start_size_var(v)
        start_size_var = v
    end
    function node:get_end_size()
        return end_size
    end
    function node:set_end_size(v)
        size_changes = true
        end_size = v
    end
    function node:get_end_size_var()
        return end_size_var
    end
    function node:set_end_size_var(v)
        end_size_var = v
    end
    function node:get_angle()
        return angle
    end
    function node:set_angle(v)
        angle = v
    end
    function node:get_angle_var()
        return angle_var
    end
    function node:set_angle_var(v)
        angle_var = v
    end
    function node:get_speed()
        return speed
    end
    function node:set_speed(v)
        speed = v
    end
    function node:get_speed_var()
        return speed_var
    end
    function node:set_speed_var(v)
        speed_var = v
    end
    function node:get_life()
        return life
    end
    function node:set_life(v)
        life = v
    end
    function node:get_life_var()
        return life_var
    end
    function node:set_life_var(v)
        life_var = v
    end
    function node:get_start_color()
        return vec4(start_r, start_g, start_b, start_a)
    end
    function node:set_start_color(v)
        start_r = v.r
        start_g = v.g
        start_b = v.b
        start_a = v.a
    end
    function node:get_start_color_var()
        return vec4(start_r_var, start_g_var, start_b_var, start_a_var)
    end
    function node:set_start_color_var(v)
        start_r_var = v.r
        start_g_var = v.g
        start_b_var = v.b
        start_a_var = v.a
    end
    function node:get_end_color()
        return vec4(end_r, end_g, end_b, end_a)
    end
    function node:set_end_color(v)
        color_changes = true
        end_r = v.r
        end_g = v.g
        end_b = v.b
        end_a = v.a
    end
    function node:get_end_color_var()
        return vec4(end_r_var, end_g_var, end_b_var, end_a_var)
    end
    function node:set_end_color_var(v)
        end_r_var = v.r
        end_g_var = v.g
        end_b_var = v.b
        end_a_var = v.a
    end
    function node:get_emission_rate()
        return emission_rate
    end
    function node:set_emission_rate(v)
        emission_rate = v
    end
    function node:get_start_particles()
        return start_particles
    end
    function node:get_max_particles()
        return max_particles
    end
    function node:get_gravity()
        return vec2(gravity_x, gravity_y)
    end
    function node:set_gravity(v)
        gravity_x = v.x
        gravity_y = v.y
        gravity_x_2 = gravity_x / 2
        gravity_y_2 = gravity_y / 2
    end
    function node:get_damping()
        return damping
    end
    function node:set_damping(v)
        damping = v
    end
    function node:get_sprite()
        return sprite
    end
    function node:set_sprite(s)
        if not sprite then
            error("cannot set particle system sprite if it was not initialized with a sprite")
        end
        sprite = s
        uv_offset = vec2(sprite.s1, sprite.t1)
        uv_scale = vec2(sprite.s2 - sprite.s1, sprite.t2 - sprite.t1)
        node"bind".tex = sprite.texture
        node"bind".uv_offset = uv_offset
        node"bind".uv_scale = uv_scale
    end
    function node:get_active_particles()
        return n
    end
    function node:reset()
        init()
    end

    node:tag"particles2d"

    return node
end
