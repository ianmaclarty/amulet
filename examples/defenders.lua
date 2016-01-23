win = am.window{
    title = "Defenders of the Weeping Quasar",
    width = 640, height = 480,
}

-- constants

ship_start_pos = vec2(0, -100)
ship_min_position = vec2(-310, -230)
ship_max_position = -ship_min_position
ship_speed = 300
ship_radius = 5
laser_velocity = vec2(0, 600)
laser_interval = 0.1

-- sounds

laser_sound = 86572901
explode_sound_slug = 54429702
explode_sound_tank = 71980902
explode_sound_worm = 6816502--64989002
explode_sound_boss = 83275102
spawn_sound_slug = 90404206
spawn_sound_tank = 48219003
spawn_sound_worm = 56680800
spawn_sound_boss = 42319309
ship_explode_sound = 80020902
ship_explode_sound2 = 11085802
hit_sound = 9975402
boss_hit_sound = 5734004
boss_sound = 31642009
boss_shoot_sound = 90380302
start_sound = 24012402
title_sound = 35270309

-- global state

global_state = am.load_state("defenders_state") or {
    high_score = 0
}

-- utility functions

function animated_sprite(frames, delay)
    local frame = 1
    local t = am.frame_time
    return am.sprite(frames[1]):action(function(sprite)
        if am.frame_time - t >= delay then
            frame = frame % #frames + 1
            sprite.source = frames[frame]
            t = am.frame_time
        end
    end)
end

function shake_screen(state, amount, duration)
    local last_shake_t = 0
    local start_t = am.frame_time
    state.camera:action("shake", function(scene)
        if am.frame_time - start_t > duration then
            scene.position2d = vec2(0)
            return true
        end
        if am.frame_time - last_shake_t > 0.05 then
            scene.position2d = math.randvec2() * amount
            last_shake_t = am.frame_time
        end
    end)
end

-- explosions 

explosion_img = [[
...
.W.
...]]

function spawn_explosion(state, position, size, color, color_var, sound)
    local explosion_particles =
        am.blend("add")
        ^ am.particles2d{
            sprite_source = explosion_img,           
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
    local radius = size * 20 + math.random() * 10
    local circle1 = am.circle(position, radius, vec4(1))
    local circle2 = am.circle(position - vec2(radius * 0.2, 0), radius * 0.8, vec4(0, 0, 0, 1))
    local flash = am.rect(
        win.left - 50, win.bottom - 50, win.right + 50, win.top + 50,
        color * vec4(0.25, 0.25, 0.25, 1))
    state.explosions_group:append(explosion_particles)
    state.explosions_group:append(circle1)
    state.scene:action(am.series{
        am.delay(0.1),
        function()
            state.explosions_flash_group:append(flash)
            circle1.color = color + vec4(0.5)
            state.explosions_group:append(circle2)
            return true
        end,
        am.delay(0.1),
        function()
            state.explosions_flash_group:remove(flash)
            state.explosions_group:remove(circle1)
            state.explosions_group:remove(circle2)
            return true
        end,
        function()
            if explosion_particles"particles2d".active_particles == 0 then
                state.explosions_group:remove(explosion_particles)
                return true
            end
        end
    })
    local pitch = math.random() * 0.5 + 0.75
    main_scene:action(am.play(sound, false, pitch))
    shake_screen(state, size * 8, size * 0.1)
end

-- slug enemy

slug_frame1 = [[
GGGGGGGG
GggggggG
GgGGGGgG
GgGMMGgG
GgGMMGgG
GgGGGGgG
GggggggG
GGGGGGGG
]]

slug_frame2 = [[
gggggggg
gGGGGGGg
gGggggGg
gGgmmgGg
gGgmmgGg
gGggggGg
gGGGGGGg
gggggggg
]]

slug_hit_frame = slug_frame1:gsub("[^%.%s]", "W")

slug_sprite = animated_sprite({slug_frame1, slug_frame2}, 0.5)

slug_hit_sprite = am.sprite(slug_hit_frame)

function slug_update(enemy, state)
    local dt = am.delta_time
    local pos = enemy.position
    local t = am.frame_time - enemy.t
    local vx = math.sin(t*3) * 150
    local vy = -80
    pos = pos + vec2(vx, vy) * dt
    enemy.position = pos
    enemy.node.position2d = pos
    if am.frame_time > enemy.last_fire_time + 1.1 then
        local vel = math.normalize(state.ship.position - pos) * 150
        spawn_bullet(state, pos, vel)
        enemy.last_fire_time = am.frame_time
    end
end

function slug_hit(enemy)
    local node = enemy.node
    node"scale":remove_all()
    node"scale":append(slug_hit_sprite)
    node:action(am.play(hit_sound))
    node:action(am.series{am.delay(0.1), function()
        node"scale":remove_all()
        node"scale":append(slug_sprite)
        return true
    end})
end

function slug(state, x)
    local position = vec2(x, win.top + 25)
    local node = am.translate(position)
        ^ am.scale(4)
        ^ slug_sprite
    return {
        node = node,
        update = slug_update,
        position = position,
        last_fire_time = am.frame_time,
        hp = 1,
        points = 200,
        t = am.frame_time,
        radius = 20,
        explosion_size = 1,
        explosion_sound = explode_sound_slug,
        spawn_sound = spawn_sound_slug,
        explosion_color = vec4(0.1, 1, 0.1, 1),
        explosion_color_var = vec4(0.2, 0.2, 0.2, 0),
        hit = slug_hit,
    }
end

-- tank enemy

tank_frame1 = [[
...r.r.r...
.rrrrrrrrr.
.rRrrrrrRr.
rrrrMMMrrrr
.rrMMmMMrr.
rrrMmmmMrrr
.rrMMmMMrr.
rrrrMMMrrrr
.rRrrrrrRr.
.rrrrrrrrr.
...r.r.r...
]]

tank_frame2 = [[
...r.r.r...
.rrrrrrrrr.
.rrrrRrrrr.
rrrrmmmrrrr
.rrmmMmmrr.
rrRmMMMmRrr
.rrmmMmmrr.
rrrrmmmrrrr
.rrrrRrrrr.
.rrrrrrrrr.
...r.r.r...
]]

tank_hit_frame = tank_frame1:gsub("[^%.%s]", "W")

tank_sprite = animated_sprite({tank_frame1, tank_frame2}, 0.5)

tank_hit_sprite = am.sprite(tank_hit_frame)

function tank_update(enemy, state)
    local dt = am.delta_time
    local pos = enemy.position
    local t = am.frame_time - enemy.t
    local phase_time = 0.5
    local phase = math.floor(t / phase_time) % 8
    local vx, vy = 0, 0
    if phase == 0 or phase == 4 then
        vx = 0
        vy = -160
    elseif phase == 2 then
        vx = 200
        vy = 0
    elseif phase == 6 then
        vx = -200
        vy = 0
    end
    pos = pos + vec2(vx, vy) * dt
    enemy.position = pos
    enemy.node.position2d = pos
    if am.frame_time > enemy.last_fire_time + phase_time * 2 then
        local vel = math.normalize(state.ship.position - pos) * 200
        spawn_bullet(state, pos, vel)
        enemy.last_fire_time = am.frame_time
    end
end

function tank_hit(enemy)
    local node = enemy.node
    node:replace("sprite", tank_hit_sprite)
    node:action(am.play(hit_sound))
    node:action(am.series{am.delay(0.1), function()
        node:replace("sprite", tank_sprite)
        return true
    end})
    enemy.position = enemy.position + vec2(0, 10)
end

function tank(state, x)
    local position = vec2(x, win.top + 30)
    local node = am.translate(position)
        ^ am.scale(4)
        ^ tank_sprite
    return {
        node = node,
        update = tank_update,
        position = position,
        last_fire_time = am.frame_time,
        hp = 6,
        points = 500,
        t = am.frame_time,
        radius = 25,
        explosion_size = 1.5,
        explosion_sound = explode_sound_tank,
        spawn_sound = spawn_sound_tank,
        explosion_color = vec4(1, 0.1, 0.1, 1),
        explosion_color_var = vec4(0.2, 0.2, 0.2, 0),
        hit = tank_hit,
    }
end

-- worm enemy

worm_frame1 = [[
...b...
..bBb..
.bBCBb.
bBCCCBb
.bBCBb.
..bBb..
...b...
]]

worm_frame2 = [[
...C...
..CBC..
.CBbBC.
CBbbbBC
.CBbBC.
..CBC..
...C...
]]

worm_hit_frame = worm_frame1:gsub("[^%.%s]", "W")

worm_sprite = animated_sprite({worm_frame1, worm_frame2}, 0.5)

worm_hit_sprite = am.sprite(worm_hit_frame)

function worm_update(enemy, state)
    local dt = am.delta_time
    local pos = enemy.position
    local center = enemy.center
    local dir = enemy.dir
    local t = am.frame_time - enemy.t
    local vx, vy = dir * 80, -80
    center = center + vec2(vx, vy) * dt
    local radius = 50
    pos = center + vec2(math.cos(t*4*dir) * radius, math.sin(t*4*dir) * radius)
    enemy.center = center
    enemy.position = pos
    enemy.node.position2d = pos
end

function worm_hit(enemy)
    local node = enemy.node
    node"scale":remove_all()
    node"scale":append(worm_hit_sprite)
    node:action(am.play(hit_sound))
    node:action(am.series{am.delay(0.1), function()
        node"scale":remove_all()
        node"scale":append(worm_sprite)
        return true
    end})
end

function worm(state, x, dir)
    local position = vec2(x, win.top+20)
    local node = am.translate(position)
        ^ am.scale(4)
        ^ worm_sprite
    return {
        node = node,
        dir = dir,
        update = worm_update,
        position = position,
        center = position,
        last_fire_time = am.frame_time,
        hp = 1,
        points = 100,
        t = am.frame_time,
        radius = 15,
        explosion_size = 0.8,
        explosion_sound = explode_sound_worm,
        spawn_sound = spawn_sound_worm,
        explosion_color = vec4(0.2, 1, 1, 1),
        explosion_color_var = vec4(0.2, 0.2, 0.2, 0),
        hit = worm_hit,
    }
end

-- boss

boss_frame1 = [[
...............w...............
....kwkkkwkwwkwww...k...w......
....w.....k...kmkkkkwwwkk......
.wwwwkk...w...kMk...w...kkkw...
......kkwwkkwwwmwwkkww..w......
......w.......wMw...w......k...
...kwwkwww....kmk...w..wwwkkk..
......k.......kMk...w...k..k...
..wwwkwkwwwkwwwmwwkwwkkkk......
....w......k..wMw.......k......
....k......w..kmk...kkwwkwwww..
....k...wwkkkwkMk....w....k....
....k.....k...wmw....k....w....
....w.....w..wwMww...w....w....
.wkkwwkkwwkkwwmMmwwkkwwkkwwkkw.
wwmMmMmMmMmMmMMWMMmMmMmMmMmMmww
.wkkwwkkwwkkwwmMmwwkkwwkkwwkkw.
....w........wwMww..w....w.w...
....k..k......wmw...w....w.w...
..kwwwkwkkwwkkkMkwkkk..w.......
.......k......kmk...w.wkw...w..
.kkkkkkwkkwwkkwMw...w..k....k..
.......k......wmw.wkwwwkwwwkkw.
....kkwwwkwwwkkMk..w...k....k..
.......w..k...kmk..w..wkw...w..
...k...w..w...wMw..w...w.......
.wwkkwwk......wmw..k.......w...
...k...k......kMkwkkkwwwkwwkw..
.......wkwwwkwkmk..k....w..w...
.......k..k...www..w....k......
...............w...............
]]

boss_frame2 = [[
...............w...............
....kwkkkwkwwkwww...k...w......
....w.....k...kMkkkkwwwkk......
.wwwwkk...w...kmk...w...kkkw...
......kkwwkkwwwMwwkkww..w......
......w.......wmw...w......k...
...kwwkwww....kMk...w..wwwkkk..
......k.......kmk...w...k..k...
..wwwkwkwwwkwwwMwwkwwkkkk......
....w......k..wmw.......k......
....k......w..kMk...kkwwkwwww..
....k...wwkkkwkmk....w....k....
....k.....k...wMw....k....w....
....w.....w..wwmww...w....w....
.wkkwwkkwwkkwwMMMwwkkwwkkwwkkw.
wwMmMmMmMmMmMmMWMmMmMmMmMmMmMww
.wkkwwkkwwkkwwMMMwwkkwwkkwwkkw.
....w........wwmww..w....w.w...
....k..k......wMw...w....w.w...
..kwwwkwkkwwkkkmkwkkk..w.......
.......k......kMk...w.wkw...w..
.kkkkkkwkkwwkkwmw...w..k....k..
.......k......wMw.wkwwwkwwwkkw.
....kkwwwkwwwkkmk..w...k....k..
.......w..k...kMk..w..wkw...w..
...k...w..w...wmw..w...w.......
.wwkkwwk......wMw..k.......w...
...k...k......kmkwkkkwwwkwwkw..
.......wkwwwkwkMk..k....w..w...
.......k..k...www..w....k......
...............w...............
]]

boss_hit_frame = boss_frame1:gsub("[^%.%s]", "C")

boss_sprite = animated_sprite({boss_frame1, boss_frame2}, 0.5)

boss_hit_sprite = am.sprite(boss_hit_frame)

function boss_update(enemy, state)
    local dt = am.delta_time
    local pos = enemy.position
    local t = am.frame_time - enemy.t
    local vx, vy = 0, 0
    if t < 4 then
        return
    else
        if pos.y > 140 then
            vy = -120
        elseif pos.y > 0 then
            vy = -30
        end
        local phase = math.floor((t - 3) / 1) % 8
        if phase == 1 or phase == 7 then
            vx = 200
        elseif phase == 3 or phase == 5 then
            vx = -200
        end
    end
    pos = pos + vec2(vx, vy) * dt
    enemy.position = pos
    enemy.node"translate".position2d = pos
    if am.frame_time > enemy.last_fire_time + 1 and t > 5 then
        local n = 16
        local speed = (100 * math.random() + 200)
        for i = 0, n-1 do
            local angle = i/n*math.pi*2
            local vel = vec2(math.cos(angle), math.sin(angle)) * speed
            spawn_bullet(state, pos, vel)
        end
        enemy.node:action(am.play(boss_shoot_sound, false, 0.5, 2))
        enemy.last_fire_time = am.frame_time - math.random() * 0.5
    end
end

function boss_hit(enemy, state)
    local node = enemy.node
    node"scale":remove_all()
    node"scale":append(boss_hit_sprite)
    node:action(am.play(boss_hit_sound))
    node:action("flash", am.series{am.delay(0.05), function()
        node"scale":remove_all()
        node"scale":append(boss_sprite)
        return true
    end})
    enemy.position = enemy.position + vec2(0, 10)
    enemy.health_bar.x2 = enemy.hp / enemy.max_hp * 200 - 100
end

function boss_kill(enemy, state)
    state.no_new_enemies = false
    state.scene:action(am.series{
        am.tween(enemy.bg_color"rect", 2, {color = vec4(0.3, 0, 0, 0)}),
        function()
            state.scene:remove(enemy.bg_color)
            return true
        end
    })
    bg_stars"particles2d".start_color = vec4(0, 0, 1, 1)
    bg_stars"particles2d".speed = 600
end

function boss(state)
    state.no_new_enemies = true

    local bg_color = am.blend"alpha"
        ^ am.rect(win.left - 50, win.bottom - 50, win.right + 50, win.top + 50,
            vec4(0.3, 0, 0, 0))
    state.scene:prepend(bg_color)
    state.scene:action(am.tween(bg_color"rect", 2, {color = vec4(0.3, 0, 0, 1)}))
    bg_stars"particles2d".start_color = vec4(1, 0.5, 0.5, 1)
    bg_stars"particles2d".speed = 1200

    local position = vec2(0, win.top + 70)
    local health_bar = am.rect(-100, 210, 100, 220, vec4(1, 0, 1, 1))
    local health_bar_bg = am.rect(-102, 208, 102, 222, vec4(0.5, 0.5, 0.5, 1))

    local node =
        am.group{
            am.translate(position)
            ^ am.scale(4)
            ^ boss_sprite
            ,
            am.group{
                health_bar_bg,
                health_bar,
            }
        }
    local max_hp = 100
    node:action(am.play(boss_sound))
    return {
        node = node,
        bg_color = bg_color,
        update = boss_update,
        position = position,
        last_fire_time = am.frame_time,
        hp = max_hp,
        max_hp = max_hp,
        points = 100000,
        t = am.frame_time,
        radius = 62,
        explosion_size = 3,
        explosion_sound = explode_sound_boss,
        spawn_sound = spawn_sound_boss,
        explosion_color = vec4(1, 1, 1, 1),
        explosion_color_var = vec4(0.5, 0.5, 0.5, 0),
        num_explosions = 10,
        hit = boss_hit,
        kill = boss_kill,
        health_bar = health_bar,
    }
end

-- enemy patterns

enemy_spawn_patterns = {
    {
        {enemy = slug, args = {-200}, delay = 0.6},
        {enemy = slug, args = {-200}, delay = 0.4},
        {enemy = slug, args = {-200}, delay = 0.4},
        {enemy = slug, args = {-200}, delay = 0.4},
    },
    {
        {enemy = slug, args = {200 }, delay = 0.4},
        {enemy = slug, args = {200 }, delay = 0.4},
        {enemy = slug, args = {200 }, delay = 0.4},
        {enemy = slug, args = {200 }, delay = 0.4},
    },
    {
        {enemy = slug, args = {0},    delay = 0.4},
        {enemy = slug, args = {0},    delay = 0.4},
        {enemy = slug, args = {0},    delay = 0.4},
        {enemy = slug, args = {0},    delay = 0.4},
    },
    {
        {enemy = tank, args = {50}, delay = 1},
        {enemy = tank, args = {50}, delay = 1},
        {enemy = tank, args = {50}, delay = 1},
        {enemy = tank, args = {50}, delay = 1},
    },
    {
        {enemy = tank, args = {-120}, delay = 1},
        {enemy = tank, args = {-120}, delay = 1},
        {enemy = tank, args = {-120}, delay = 1},
        {enemy = tank, args = {-120}, delay = 1},
    },
    {
        {enemy = worm, args = {-220, 1}, delay = 0.3},
        {enemy = worm, args = {-220, 1}, delay = 0.1},
        {enemy = worm, args = {-220, 1}, delay = 0.1},
        {enemy = worm, args = {-220, 1}, delay = 0.1},
        {enemy = worm, args = {-220, 1}, delay = 0.1},
        {enemy = worm, args = {-220, 1}, delay = 0.1},
        {enemy = worm, args = {-220, 1}, delay = 0.1},
    },
    {
        {enemy = worm, args = {220, -1}, delay = 0.3},
        {enemy = worm, args = {220, -1}, delay = 0.1},
        {enemy = worm, args = {220, -1}, delay = 0.1},
        {enemy = worm, args = {220, -1}, delay = 0.1},
        {enemy = worm, args = {220, -1}, delay = 0.1},
        {enemy = worm, args = {220, -1}, delay = 0.1},
        {enemy = worm, args = {220, -1}, delay = 0.1},
    },
    {
        {enemy = worm, args = {220, -1}, delay = 3},
    },
    {
        {enemy = boss, args = {0}, delay = 3},
    }
}

pattern_sets = {
    { 1, 2, 3, 1, 6, 7, 6, 7, },
    { 1, 2, 3, 3, 4, 5, 4, 5, 6, 7, },
    { 1, 2, 3, 1, 2, 6, 7, 6, 7, },
    { 9 },
    { 8 },
}

-- enemy bullets

bullet_sprite = am.sprite[[
.mmm.
mmMmm
mMWMm
mmMmm
.mmm.
]]

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

-- general enemy logic

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
            local sound = enemy.explosion_sound
            spawn_explosion(state, pos, size, color, color_var, sound)
            state.score = state.score + enemy.points
            if enemy.kill then
                enemy:kill(state)
            end
            if (enemy.num_explosions or 1) > 1 then
                state.explosions_group:action(coroutine.create(function()
                    for i = 2, enemy.num_explosions do
                        local explosion_pos = pos + 
                            vec2(math.random() * 2 - 1, math.random() * 2 - 1)
                            * enemy.radius
                        spawn_explosion(state, explosion_pos, size, color, color_var, sound)
                        am.wait(am.delay(math.random() * 0.2 + 0.05))
                    end
                end))
            end
        end
        if enemy.hp <= 0 or
            pos.y < win.bottom - 100 or
            pos.y > win.top + 100 or
            pos.x < win.left - 100 or
            pos.x > win.right + 100 
        then
            state.enemies_group:remove(enemy.node) 
            table.remove(enemies, i)
        else
            enemy.position = pos
            enemy.node.position2d = pos
        end
    end
end

function spawn_enemies(state)
    if state.no_new_enemies or state.dead then
        state.last_enemy_spawn_time = am.frame_time
        return
    end
    local pat = enemy_spawn_patterns[state.current_pattern]
    local phase = state.pattern_phase
    local last_t = state.last_enemy_spawn_time
    if am.frame_time > last_t + pat[phase].delay then
        -- spawn next enemy
        local enemy = pat[phase].enemy(state, unpack(pat[phase].args))
        state.enemies_group:append(enemy.node)
        table.insert(state.enemies, enemy)
        state.enemies_group:action(am.play(enemy.spawn_sound, false, 2 ^ (math.random(8) / 12)))
        state.last_enemy_spawn_time = am.frame_time
        phase = phase + 1
        if phase > #pat then
            -- choose new pattern
            local pool = state.pattern_pool
            if #pool == 0 then
                -- next patt
                state.pattern_set = state.pattern_set % #pattern_sets + 1
                table.append(pool, pattern_sets[state.pattern_set])
            end
            local r = math.random(#pool)
            state.current_pattern = pool[r]
            table.remove(pool, r)
            phase = 1
        end
        state.pattern_phase = phase
    end
end

-- player ship

ship_sprite = am.sprite[[
...W...
R.WWW.R
W.WBW.W
WWWBWWW
..WWW..
]]

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
    local vel
    if dir_x ~= 0 or dir_y ~= 0 then
        local vel = math.normalize(vec2(dir_x, dir_y)) * ship_speed
        pos = pos + dt * vel
    end
    pos = math.clamp(pos, ship_min_position, ship_max_position)
    state.ship.position = pos
    state.ship.node.position2d = state.ship.position

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

laser_sprite = am.sprite[[
.YYYYY.
YOOOOOY
ORRRRRO
R.....R
.......
R.....R
]]

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
            if enemy.position.y - enemy.radius < win.top and
                math.distance(enemy.position, pos) < enemy.radius + 5
            then
                enemy.hp = enemy.hp - 1
                if enemy.hit then
                    enemy:hit(state)
                end
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
        state.lasers_group:action(am.play(laser_sound))
        -- kickback
        state.ship.position = state.ship.position + vec2(0, -4)
    end
end

-- score

function update_score(state)
    state.score_node"text".text = state.score
end

-- background stars

star_img = [[
...
.W.
...]]

bg_stars = am.blend("add")
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

-- title screen

function create_title_scene()
    local line1 = am.text("DEFENDERS", vec4(1, 0, 1, 1))
    local line2 = am.text("OF THE", vec4(1, 0, 1, 1))
    local line3 = am.text("WEEPING QUASAR", vec4(1, 0, 1, 1))
    local line4 = am.text("by Ian MacLarty", vec4(0.5, 0, 1, 1))
    local title = am.group{
        am.translate(0, 150) ^ am.scale(5) ^ line1,
        am.translate(0, 80) ^ am.scale(3) ^ line2,
        am.translate(0, 20) ^ am.scale(4) ^ line3,
        am.translate(0, -50) ^ am.scale(2) ^ line4,
    }:action(coroutine.create(function()
        local colors = {vec4(1, 0, 1, 1), vec4(0, 1, 1, 1), vec4(1, 1, 0, 1), vec4(0, 1, 0, 1)}
        local i = 1
        while true do
            line1.color = colors[i]
            line2.color = colors[i]
            line3.color = colors[i]
            am.wait(am.delay(0.1))
            i = i % #colors + 1
        end
    end))
    local score = 
        am.translate(0, -100)
        ^ am.text("HI SCORE: "..global_state.high_score, vec4(0, 1, 1, 1))
    local instructions =
        am.translate(0, -150)
        ^ am.text("ARROW KEYS: MOVE\nX: FIRE\nPRESS X TO START", vec4(0.75, 0.75, 0.75, 1))
    return am.group{
        bg_stars,
        title,
        score,
        instructions
    }:action(function()
        if win:key_pressed"x" then
            start_game()
        end
    end)
    :action(coroutine.create(function()
        while true do
            am.wait(am.play(title_sound, false, 1, 1))
            am.wait(am.play(title_sound, false, 2^(-2/12), 1))
        end
    end))
end

-- game start/end logic

function start_game()
    local ship = init_ship()
    local lasers_group = am.group()
    local bullets_group = am.group()
    local enemies_group = am.group()
    local explosions_group = am.group()
    local explosions_flash_group = am.group()
    local score_node = am.translate(300, 230)
        ^ am.scale(2)
        ^ am.text("0", "right", "top")
    local scene = am.group{
        explosions_flash_group,
        bg_stars,
        lasers_group,
        enemies_group,
        explosions_group,
        ship.node,
        bullets_group,
        score_node,
    }
    local camera = am.translate(0, 0) ^ scene
    local state = {
        score = 0,
        score_node = score_node,
        scene = scene,
        camera = camera,
        ship = ship,
        lasers = {},
        lasers_group = lasers_group,
        last_laser_time = 0,
        bullets = {},
        bullets_group = bullets_group,
        enemies = {},
        enemies_group = enemies_group,
        explosions_group = explosions_group,
        explosions_flash_group = explosions_flash_group,
        current_pattern = pattern_sets[1][1],
        pattern_phase = 1,
        pattern_set = 1,
        last_enemy_spawn_time = 0,
        pattern_pool = {},
    }
    table.append(state.pattern_pool, pattern_sets[1])
    table.remove(state.pattern_pool, 1)
    local
    function main_action()
        update_ship(state)
        update_lasers(state)
        update_enemies(state)
        spawn_enemies(state)
        update_bullets(state)
        update_score(state)
        if state.dead and am.frame_time - state.death_time > 2.5 then
            if state.score > global_state.high_score then
                global_state.high_score = state.score
                am.save_state("defenders_state", global_state)
            end
            main_scene:remove_all()
            main_scene:append(create_title_scene())
        end
    end
    scene:action(am.series{
        am.tween(ship.node"translate", 0.5, {position2d = vec2(ship_start_pos)}),
        main_action
    })
    main_scene:remove_all()
    main_scene:append(camera)
    main_scene:action(am.play(start_sound))
end

function end_game(state)
    bg_stars"particles2d".start_color = vec4(0, 0, 1, 1)
    bg_stars"particles2d".speed = 600
    local flash = am.rect(
        win.left - 50, win.bottom - 50, win.right + 50, win.top + 50,
        vec4(0, 1, 1, 1))
    state.explosions_flash_group:append(flash)
    state.dead = true
    state.death_time = am.frame_time
    -- pause briefly
    state.scene.paused = true
    main_scene:action(am.series{
        am.delay(0.3),
        function()
            state.scene.paused = false
            state.explosions_flash_group:remove(flash)
            state.scene:remove(state.ship.node)
            spawn_explosion(state, state.ship.position, 2, vec4(1, 0.7, 0.3, 1), vec4(0.2, 0.4, 0.1, 0), ship_explode_sound2)
            return true
        end
    })
    main_scene:action(am.play(ship_explode_sound))
end

-- set up the scene

main_scene = am.group{create_title_scene()}
win.scene = main_scene

win.scene:action(function()
    if win:key_pressed"escape" then
        win:close()
    end
end)
