math.randomseed(os.time())

local color = vec4(math.random() * 0.8, math.random() * 0.8, math.random() * 0.8, 1)
local radius = 30

local win = am.window{
    title = "waiting for script...",
    letterbox = false,
}

local pp = am.postprocess{
    auto_clear = false,
    clear_color = color,
}

local rect = am.translate(vec3(-20, 0, 0))
    ^ am.rect(-radius, -radius, radius, radius, color)

local speed = vec2(100, 100)

rect:action(function()
    local pos = rect"translate".position
    local w, h = win.pixel_width, win.pixel_height
    if pos.x < -w/2 and speed.x < 0 or pos.x > w/2 and speed.x > 0 then
        speed = speed{x = -speed.x}
    end
    if pos.y < 0 and speed.y < 0 or pos.y > h and speed.y > 0 then
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
    pp.clear_color = color
end)

win.scene =
    pp:action(function(cam)
        local w, h = win.pixel_width, win.pixel_height
        win.projection = math.ortho(-w/2, w/2, 0, h)
    end)
    ^ {
        rect,
        am.translate(0, 40)
        ^ am.scale(vec3(2))
        ^ am.text("Amulet v"..am.version.."\nwaiting for script...", "center", "bottom")
    }

