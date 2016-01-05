local win = am.window{
    title = "sfxr",
    width = 320, height = 240}

math.randomseed(os.time())

local colors = {
    vec4(1, 1, 0, 1),
    vec4(1, 0, 0, 1),
    vec4(0.8, 0.8, 0.8, 1),
    vec4(0, 1, 0, 1),
    vec4(0.0, 0.5, 1, 1),
    vec4(0.5, 0.3, 0.9, 1),
    vec4(0, 0.5, 0, 1),
    vec4(0, 1, 1, 1),
    vec4(1, 0, 1, 1),
    vec4(1, 0.5, 0, 1),
}

local black = vec4(0, 0, 0, 1)
local white = vec4(1)

local add_to_list

local
function button(x, y, label, gen, n)
    local x1, y1, x2, y2 = -90, -10, 4, 6
    local node = am.translate(x, y)
        ^ {
            am.rect(-90, -10, 4, 6, colors[n % #colors + 1]),
            am.text(label, black, "right")
        }
    node:action(function()
        if win:mouse_pressed"left" then
            local pos = win:mouse_position()
            local bpos = node.position2d
            if pos.x - bpos.x > x1 and pos.x - bpos.x < x2 and
                pos.y - bpos.y > y1 and pos.y - bpos.y < y2 
            then
                local seed
                if gen then
                    seed = math.random(1000000) * 100 + n
                else
                    seed = n
                end
                local buf = am.gen_sfx_buffer(seed)
                node:action(am.play(am.track(buf, false, 1)))
                print("seed: "..seed)
                if gen then
                    add_to_list(seed)
                end
            end
        end
    end)
    return node
end

local list = am.group()

function add_to_list(seed)
    if list.num_children >= 10 then
        for i, child in list:child_pairs() do
            child.position2d = vec2(120, 100 - (i-2) * 20)
        end
        list:remove(list:child(1))
        list:append(button(120, -80, seed, false, seed))
    else
        list:append(button(120, 100 - list.num_children * 20, seed, false, seed))
    end
end

win.scene = am.group{
    button(-30, 100, "PICKUP/COIN", true, 0),
    button(-30,  80, "LASER/SHOOT", true, 1),
    button(-30,  60, "EXPLOSION",   true, 2),
    button(-30,  40, "POWERUP",     true, 3),
    button(-30,  20, "HIT/HURT",    true, 4),
    button(-30,   0, "JUMP",        true, 5),
    button(-30, -20, "BLIP/SELECT", true, 6),
    button(-30, -40, "BIRD",        true, 7),
    button(-30, -60, "PUSH",        true, 8),
    button(-30, -80, "RANDOM",      true, 9),
    list,
}