noglobals()
local win = am.window{}


local
function make_particles()
    local max_particles = 1000
    local emission_rate = 500
    local gravity = vec2(0, -200)

    local particle_quads = am.quads(max_particles, {"vert", "vec2", "color", "ubyte_norm4"})
    local quad_vert = particle_quads.vert
    local quad_color = particle_quads.color
    local data = mathv.struct_of_arrays(max_particles, {
        "position", "vec2",
        "velocity", "vec2",
        "size", "float",
        "t", "float"
    })

    local time_to_live = 1.5

    local particles = mathv.view_group{
        position = data.position,
        velocity = data.velocity,
        size = data.size,
        t = data.t,
        vert1 = quad_vert:slice(1, nil, 4),
        vert2 = quad_vert:slice(2, nil, 4),
        vert3 = quad_vert:slice(3, nil, 4),
        vert4 = quad_vert:slice(4, nil, 4),
        color1 = quad_color:slice(1, nil, 4),
        color2 = quad_color:slice(2, nil, 4),
        color3 = quad_color:slice(3, nil, 4),
        color4 = quad_color:slice(4, nil, 4),
    }

    local num_active = 0
    local t_accum = 0

    local
    function update()
        local active_particles = particles:slice(1, num_active)

        -- remove dead particles
        am.buffer_pool(function()
            local alive = mathv.gt(active_particles.t, 0)
            num_active = active_particles:filter(alive)
        end)

        -- spawn new particles
        t_accum = t_accum + am.delta_time
        local num_to_spawn = math.min(max_particles - num_active, math.floor(t_accum * emission_rate))
        if num_to_spawn > 0 then
            t_accum = t_accum - num_to_spawn / emission_rate
            am.buffer_pool(function()
                local new_particles = particles:slice(num_active + 1, num_to_spawn)
                new_particles.position = win:mouse_position()
                new_particles.size = 4
                local angle = mathv.random("float", num_to_spawn) * (math.pi * 2)
                local speed = mathv.random("float", num_to_spawn) * 60 + 50
                local vel = mathv.vec2(mathv.cos(angle) * speed, mathv.sin(angle) * speed)
                new_particles.velocity = vel
                new_particles.t = 1
                local color = mathv.vec4(
                    mathv.random("float", num_to_spawn), 
                    mathv.random("float", num_to_spawn), 
                    mathv.random("float", num_to_spawn), 1)
                new_particles.color1 = color
                new_particles.color2 = color
                new_particles.color3 = color
                new_particles.color4 = color
            end)
            num_active = num_active + num_to_spawn
        end

        active_particles = particles:slice(1, num_active)

        -- update particles
        am.buffer_pool(function()
            active_particles.position = active_particles.position + active_particles.velocity * am.delta_time
            active_particles.velocity = active_particles.velocity + gravity * am.delta_time
            active_particles.size = 4 * active_particles.t

            active_particles.vert1 = active_particles.position + mathv.vec2(-active_particles.size, active_particles.size)
            active_particles.vert2 = active_particles.position + mathv.vec2(-active_particles.size, -active_particles.size)
            active_particles.vert3 = active_particles.position + mathv.vec2(active_particles.size, -active_particles.size)
            active_particles.vert4 = active_particles.position + mathv.vec2(active_particles.size, active_particles.size)

            active_particles.t = active_particles.t - am.delta_time / time_to_live
        end)

        -- only draw active particles
        particle_quads"draw".count = num_active * 6

        --log(particles.position[num_active])
        --print(num_active)
    end

    local node = am.blend"add" ^ am.use_program(am.shaders.colors2d) ^ particle_quads
    node:action(update)

    return node
end

win.scene = make_particles()
