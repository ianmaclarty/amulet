local color = vec4(0.8, 0, 0.5, 1)
local radius = 7

local win = am.window{
    title = "waiting for script...",
    auto_clear = false,
    clear_color = color
}

math.randomseed(os.time())

local rect = am.translate(vec3(-20, 0, 0))
    ^ am.rect(-radius, -radius, radius, radius, color)

local speed = vec2(37, 43)

rect:action(function()
    local pos = rect"translate".position
    if pos.x < -100 and speed.x < 0 or pos.x > 100 and speed.x > 0 then
        speed = speed{x = -speed.x}
    end
    if pos.y < -50 and speed.y < 0 or pos.y > 50 and speed.y > 0 then
        speed = speed{y = -speed.y}
    end
    pos = pos + vec3(speed, 0) * am.delta_time
    rect"translate".position = pos
    color = vec4(
        color.r + (math.random() * 4 - 2) * am.delta_time,
        color.g + (math.random() * 4 - 2) * am.delta_time,
        color.b + (math.random() * 4 - 2) * am.delta_time,
        1
    )
    color = math.clamp(color, 0, 1)
    rect"rect".color = color
    win.clear_color = color
end)

win.scene =
    am.camera2d(200, 100)
    ^ {
        rect,
        am.translate(vec3(0, -50, 0))
        ^ am.text("Amulet v"..am.version.."\nwaiting for script...", "center", "bottom")
    }
