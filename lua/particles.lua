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
--      sprite
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
    local end_size = (opts.end_size or 20) / 2
    local end_size_var = (opts.end_size_var or 0) / 2
    local angle = opts.angle or 0
    local angle_var = opts.angle_var or math.rad(30)
    local speed = opts.speed or 100
    local speed_var = opts.speed_var or 10
    local life = opts.life or 1
    local life_var = opts.life_var or 0.1
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
    local end_r = opts.end_color and opts.end_color.r or 1
    local end_g = opts.end_color and opts.end_color.g or 1
    local end_b = opts.end_color and opts.end_color.b or 1
    local end_a = opts.end_color and opts.end_color.a or 1
    local end_r_var = opts.end_color_var and opts.end_color_var.r or 0
    local end_g_var = opts.end_color_var and opts.end_color_var.g or 0
    local end_b_var = opts.end_color_var and opts.end_color_var.b or 0
    local end_a_var = opts.end_color_var and opts.end_color_var.a or 0
    local emission_rate = opts.emission_rate or 50
    local gravity_x = opts.gravity and opts.gravity.x or 0
    local gravity_y = opts.gravity and opts.gravity.y or 0
    local gravity_x_2 = gravity_x / 2
    local gravity_y_2 = gravity_y / 2
    local sprite = opts.sprite

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
        x1_view:set(x, n)
        y1_view:set(y, n)
        z1_view:set(z, n)
        xyz2_view:set(xyz1_view, n)
        xyz3_view:set(xyz1_view, n)
        xyz4_view:set(xyz1_view, n)
        r1_view:set(r, n)
        g1_view:set(g, n)
        b1_view:set(b, n)
        a1_view:set(a, n)
        rgba2_view:set(rgba1_view, n)
        rgba3_view:set(rgba1_view, n)
        rgba4_view:set(rgba1_view, n)
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
        dz[n] = (end_size + (rnd() * 2 - 1) * end_size_var - z[n]) / time_to_live[n]
        r[n] = start_r + (rnd() * 2 - 1) * start_r_var
        g[n] = start_g + (rnd() * 2 - 1) * start_g_var
        b[n] = start_b + (rnd() * 2 - 1) * start_b_var
        a[n] = start_a + (rnd() * 2 - 1) * start_a_var
        dr[n] = (end_r + (rnd() * 2 - 1) * end_r_var - r[n]) / time_to_live[n]
        dg[n] = (end_g + (rnd() * 2 - 1) * end_g_var - g[n]) / time_to_live[n]
        db[n] = (end_b + (rnd() * 2 - 1) * end_b_var - b[n]) / time_to_live[n]
        da[n] = (end_a + (rnd() * 2 - 1) * end_a_var - a[n]) / time_to_live[n]
        local speed = speed + (rnd() * 2 - 1) * speed_var
        local angle = angle + (rnd() * 2 - 1) * angle_var
        speed_x[n] = cos(angle) * speed
        speed_y[n] = sin(angle) * speed
    end

    for i = 1, start_particles do
        add_particle()
    end
    set_verts()

    local
    function update()
        local dt = am.delta_time

        -- update existing particles
        local i = 1
        while i <= n do
            time_to_live[i] = time_to_live[i] - dt
            if time_to_live[i] >= 0 then
                x[i] = x[i] + dt * (speed_x[i] + dt * gravity_x_2)
                y[i] = y[i] + dt * (speed_y[i] + dt * gravity_y_2)
                speed_x[i] = speed_x[i] + gravity_x * dt
                speed_y[i] = speed_y[i] + gravity_y * dt
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
    
    node:action(update)

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

    node:tag"particles2d"

    return node
end
