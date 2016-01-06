local win = am.window{
    title = "Hello, Galaxy!",
    width = 640, height = 480
}

-- constants:
local ship_start_pos = vec2(0, -100)
local ship_min_position = vec2(-310, -230)
local ship_max_position = -ship_min_position
local ship_speed = 340
local ship_radius = 5
local laser_velocity = vec2(0, 600)
local laser_interval = 0.1

local ship_sprite = am.sprite[[
.........
....W....
.R.WWW.R.
.W.WBW.W.
.WWWBWWW.
...WWW...
.........
]]

local laser_sprite = am.sprite[[
.........
..YYYYY..
.YOOOOOY.
.ORRRRRO.
.R.....R.
.........
.R.....R.
.........
]]

local bullet_sprite = am.sprite[[
.......
..BBB..
.BBCBB.
.BCWCB.
.BBCBB.
..BBB..
.......
]]

local star_img = [[
...
.W.
...]]

local laser_buf = am.sfxr_synth(86572901)
local explode_buf = am.sfxr_synth(9975402)

local spawn_bullet

-- slug enemy

local slug_sprite = am.sprite[[
GGGGGGGG
GggggggG
GgGGGGgG
GgGBBGgG
GgGBBGgG
GgGGGGgG
GggggggG
GGGGGGGG
]]

local
function slug_update(enemy, state)
    local dt = am.delta_time
    local pos = enemy.position
    local t = am.frame_time - enemy.t
    local vx = math.sin(t*3) * 150
    local vy = -80
    pos = pos + vec2(vx, vy) * dt
    enemy.position = pos
    enemy.node.position2d = pos
    if am.frame_time > enemy.last_fire_time + 1 then
        local vel = math.normalize(state.ship.position - pos) * 150
        spawn_bullet(state, pos, vel)
        enemy.last_fire_time = am.frame_time
    end
end

local
function slug(position)
    local node = am.translate(position)
        ^ am.scale(4)
        ^ slug_sprite
    return {
        node = node,
        update = slug_update,
        position = position,
        last_fire_time = am.frame_time,
        hp = 1,
        points = 100,
        t = am.frame_time,
        radius = 20,
        explosion_size = 1,
        explosion_color = vec4(0.1, 1, 0.1, 1),
        explosion_color_var = vec4(0.2, 0.2, 0.2, 0),
    }
end

-- wraith enemy

local wraith_sprite = am.sprite[[
....................
...r................
...r...rrrrrr..r....
...rrrrrRRRRr.rr....
...R...rRBBRrrrr....
.......rRBBRrrRr....
......rrrrr..r.r....
.rrrrrR.r.r..rrRRrr.
.Rr.rrr.r.r..r.rrrr.
.r..r.r...r....r.rr.
.R....r........r..R.
]]

local
function wraith_update(enemy, state)
    local dt = am.delta_time
    local pos = enemy.position
    local t = am.frame_time - enemy.t
    local vx = math.sin(t*3) * 150
    local vy = -80
    pos = pos + vec2(vx, vy) * dt
    enemy.position = pos
    enemy.node.position2d = pos
    if am.frame_time > enemy.last_fire_time + 1 then
        local vel = math.normalize(state.ship.position - pos) * 150
        spawn_bullet(state, pos, vel)
        enemy.last_fire_time = am.frame_time
    end
end

local
function wraith(position)
    local node = am.translate(position)
        ^ am.scale(4)
        ^ wraith_sprite
    return {
        node = node,
        update = wraith_update,
        position = position,
        last_fire_time = am.frame_time,
        hp = 3,
        points = 500,
        t = am.frame_time,
        radius = 25,
        explosion_size = 1.5,
        explosion_color = vec4(1, 0.1, 0.1, 1),
        explosion_color_var = vec4(0.2, 0.2, 0.2, 0),
    }
end

local enemy_spawn_patterns = {
    {
        {enemy = slug, position = vec2(-200, 340), delay = 1},
        {enemy = slug, position = vec2(-200, 340), delay = 0.5},
        {enemy = slug, position = vec2(-200, 340), delay = 0.5},
        {enemy = slug, position = vec2(0, 340), delay = 1},
        {enemy = slug, position = vec2(0, 340), delay = 0.5},
        {enemy = slug, position = vec2(0, 340), delay = 0.5},
        {enemy = slug, position = vec2(200, 340), delay = 1},
        {enemy = slug, position = vec2(200, 340), delay = 0.5},
        {enemy = slug, position = vec2(200, 340), delay = 0.5},
    },
    {
        {enemy = slug, position = vec2(200, 340), delay = 1},
        {enemy = slug, position = vec2(200, 340), delay = 0.5},
        {enemy = slug, position = vec2(200, 340), delay = 0.5},
        {enemy = slug, position = vec2(0, 340), delay = 1},
        {enemy = slug, position = vec2(0, 340), delay = 0.5},
        {enemy = slug, position = vec2(0, 340), delay = 0.5},
        {enemy = slug, position = vec2(-200, 340), delay = 1},
        {enemy = slug, position = vec2(-200, 340), delay = 0.5},
        {enemy = slug, position = vec2(-200, 340), delay = 0.5},
    },
    {
        {enemy = wraith, position = vec2(0, 340), delay = 1},
        {enemy = wraith, position = vec2(0, 340), delay = 0.6},
        {enemy = wraith, position = vec2(0, 340), delay = 0.6},
    },
}

local enemy_frequencies = {
    1, 1, 1,
    2, 2, 2,
    3, 3,
}

local vshader = [[
    precision highp float;
    attribute vec2 vert;
    attribute vec2 uv;
    uniform mat4 MV;
    uniform mat4 P;
    varying vec2 v_uv;
    void main() {
        v_uv = uv;
        gl_Position = P * MV * vec4(vert, 0.0, 1.0);
    }
]]

local fshader = [[
    precision mediump float;
    uniform sampler2D tex;
    uniform float t;
    varying vec2 v_uv;
    void main() {
        vec4 sample = texture2D(tex, v_uv);
        gl_FragColor = vec4(fract(sample.rgb +
            vec3(v_uv.x * 40.0 + t + cos(v_uv.y * 60.0), pow(v_uv.y, 2.0) * 20.0 + t * 2.3
            + sin(v_uv.x * 50.0) * 1.0, t * 3.3)), sample.a);
        gl_FragColor = sample;
    }
]]

local prog = am.program(vshader, fshader)

local
function init_ship()
    local node = 
        am.translate(vec2(0, -260))
        ^ am.scale(4)
        ^ ship_sprite
    local ship = {
        node = node,
        position = ship_start_pos,
    }
    return ship
end

local
function spawn_explosion(state, position, size, color, color_var)
    local explosion =
        am.blend("add")
        ^ am.particles2d{
            sprite_source = star_img,           
            source_pos = position,
            source_pos_var = vec2(5, 5) * size,
            max_particles = 200 * size,
            start_particles = 200 * size,
            emission_rate = 0,
            life = 0.2,
            life_var = 0.5,
            angle_var = math.rad(180),
            speed = 200 * size,
            speed_var = 50 * size,
            start_color = color,
            start_color_var = color_var,
            start_size = 30,
            start_size_var = 10,
            end_size = 10,
            end_size_var = 5,
            warmup_time = 0.05,
        }
     state.explosions_group:append(explosion)
     state.explosions_group:action(function()
         if explosion"particles2d".active_particles == 0 then
             state.explosions_group:remove(explosion)
         end
     end)
     local pitch = math.random() * 0.8 + 0.4
     win.scene:action(am.play(explode_buf, false, pitch))
end

local bg

local
function end_game(state)
    -- pause briefly
    state.scene.paused = true
    bg.paused = true
    win.clear_color = vec4(0, 1, 1, 1)
    win.scene:action(am.series{
        am.delay(0.3),
        function()
            state.scene.paused = false
            bg.paused = false
            win.clear_color = vec4(0, 0, 0, 1)
            state.dead = true
            state.death_time = am.frame_time
            state.scene:remove(state.ship.node)
            spawn_explosion(state, state.ship.position, 2, vec4(1, 0.7, 0.3, 1), vec4(0.2, 0.4, 0.1, 0))
            return true
        end
    })
end

local
function update_ship(state)
    if state.dead then
        return
    end

    -- update ship position
    local dt = am.delta_time
    local pos = state.ship.position
    local dir_x, dir_y = 0, 0
    if win:key_down"left" then
        dir_x = -1
    elseif win:key_down"right" then
        dir_x = 1
    end
    if win:key_down"down" then
        dir_y = -1
    elseif win:key_down"up" then
        dir_y = 1
    end
    if dir_x ~= 0 or dir_y ~= 0 then
        local vel = math.normalize(vec2(dir_x, dir_y)) * ship_speed
        pos = pos + dt * vel
        pos = math.clamp(pos, ship_min_position, ship_max_position)
        state.ship.position = pos
        state.ship.node.position2d = pos
    end

    -- check for collision with bullets
    for _, bullet in ipairs(state.bullets) do
        if math.distance(bullet.position, pos) < bullet.radius + ship_radius then
            end_game(state)
            return
        end
    end

    -- check for collision with enemies
    for _, enemy in ipairs(state.enemies) do
        if math.distance(enemy.position, pos) < enemy.radius + ship_radius then
            end_game(state)
            return
        end
    end
end

local
function update_lasers(state)
    local dt = am.delta_time
    local lasers = state.lasers

    for i = #lasers, 1, -1 do
        -- update laser pos
        local laser = lasers[i]
        local pos = laser.position
        pos = pos + laser_velocity * dt

        -- check for collision with enemies
        local hit = false
        for _, enemy in ipairs(state.enemies) do
            if math.distance(enemy.position, pos) < enemy.radius + 5 then
                enemy.hp = enemy.hp - 1
                hit = true
                break
            end
        end

        if hit or pos.y > win.top + 20 then
            state.lasers_group:remove(laser.node) 
            table.remove(lasers, i)
        else
            laser.position = pos
            laser.node.position2d = pos
        end
    end

    -- add new laser
    if
        not state.dead and 
        win:key_down"x" and
        am.frame_time - state.last_laser_time > laser_interval
    then
        local pos = state.ship.position
        local laser = {
            position = pos,
            node = am.translate(pos) ^ am.scale(4) ^ laser_sprite
        }
        table.insert(lasers, laser)
        state.lasers_group:append(laser.node)
        state.last_laser_time = am.frame_time
        state.lasers_group:action(am.play(laser_buf))
    end
end

function spawn_bullet(state, position, velocity)
    local node = am.translate(position)
        ^ am.scale(4)
        ^ bullet_sprite
    local bullet = {
        node = node,
        position = position,
        velocity = velocity,
        radius = 12,
    }
    table.insert(state.bullets, bullet)
    state.bullets_group:append(node)
end

local
function update_bullets(state)
    local dt = am.delta_time
    local bullets = state.bullets

    -- update bullet positions
    for i = #bullets, 1, -1 do
        local bullet = bullets[i]
        local pos = bullet.position
        local vel = bullet.velocity
        pos = pos + vel * dt
        if pos.y > win.top + 20 or pos.y < win.bottom - 20 or
           pos.x > win.right + 20 or pos.x < win.left - 20
        then
            state.bullets_group:remove(bullet.node) 
            table.remove(bullets, i)
        else
            bullet.position = pos
            bullet.node.position2d = pos
        end
    end
end

local
function update_enemies(state)
    local dt = am.delta_time
    local enemies = state.enemies

    for i = #enemies, 1, -1 do
        local enemy = enemies[i]
        enemy:update(state)
        local pos = enemy.position
        if enemy.hp <= 0 then
            local size = enemy.explosion_size
            local color = enemy.explosion_color
            local color_var = enemy.explosion_color_var
            spawn_explosion(state, pos, size, color, color_var)
            state.score = state.score + enemy.points
        end
        if enemy.hp <= 0 or pos.y < win.bottom - 100 then
            state.enemies_group:remove(enemy.node) 
            table.remove(enemies, i)
        else
            enemy.position = pos
            enemy.node.position2d = pos
        end
    end
end

local
function spawn_enemies(state)
    local pat = enemy_spawn_patterns[state.current_pattern]
    local phase = state.pattern_phase
    local last_t = state.last_enemy_spawn_time
    if am.frame_time > last_t + pat[phase].delay then
        -- spawn next enemy
        local enemy = pat[phase].enemy(pat[phase].position)
        state.enemies_group:append(enemy.node)
        table.insert(state.enemies, enemy)
        state.last_enemy_spawn_time = am.frame_time
        phase = phase + 1
        if phase > #pat then
            -- choose new pattern
            local pool = state.pattern_pool
            if #pool == 0 then
                table.append(pool, enemy_frequencies)
            end
            local r = math.random(#pool)
            state.current_pattern = pool[r]
            table.remove(pool, r)
            phase = 1
        end
        state.pattern_phase = phase
    end
end

local
function update_score(state)
    state.score_node"text".text = state.score
end

local title_scene

local
function start_game()
    local ship = init_ship()
    local lasers_group = am.group()
    local bullets_group = am.group()
    local enemies_group = am.group()
    local explosions_group = am.group()
    local score_node = am.translate(300, 230)
        ^ am.scale(2)
        ^ am.text("0", "right", "top")
    local scene = am.group{
        lasers_group,
        enemies_group,
        explosions_group,
        ship.node,
        bullets_group,
        score_node,
    }
    local state = {
        score = 0,
        score_node = score_node,
        scene = scene,
        ship = ship,
        lasers = {},
        lasers_group = lasers_group,
        last_laser_time = 0,
        bullets = {},
        bullets_group = bullets_group,
        enemies = {},
        enemies_group = enemies_group,
        explosions_group = explosions_group,
        current_pattern = 1,
        pattern_phase = 1,
        last_enemy_spawn_time = 0,
        pattern_pool = {},
    }
    local
    function main_action()
        update_ship(state)
        update_lasers(state)
        update_enemies(state)
        spawn_enemies(state)
        update_bullets(state)
        update_score(state)
        if state.dead and am.frame_time - state.death_time > 1.1 then
            win.scene:remove(scene)
            win.scene:append(title_scene)
        end
    end
    scene:action(am.series{
        am.tween(ship.node"translate", 0.5, {position2d = vec2(ship_start_pos)}),
        main_action
    })
    win.scene:append(scene)
end

bg = am.blend("add")
    ^ am.particles2d{
        sprite_source = star_img,           
        source_pos = vec2(0, 250),
        source_pos_var = vec2(320, 0),
        max_particles = 200,
        emission_rate = 100,
        life = 1.5,
        angle = math.rad(270),
        speed = 600,
        speed_var = 200,
        start_color = vec4(0, 0, 1, 1),
        start_size = 10,
        start_size_var = 4,
        warmup_time = 1,
    }

title_scene = am.text("DEMO SHMUP\nPRESS X TO START")
title_scene:action(function()
    if win:key_pressed"x" then
        win.scene:remove(title_scene)
        start_game()
    end
end)

win.scene = am.group{bg, title_scene}

win.scene:action(function()
    if win:key_pressed"escape" then
        -- close is ignored in HTML5 build
        win:close()
    end
end)
