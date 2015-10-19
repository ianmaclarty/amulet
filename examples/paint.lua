local win = am.window{letterbox = false}

math.randomseed(os.time())

local particles = am.group()
local curr_color = vec4(math.random(), math.random(), math.random(), 0.15)

local
function add_particle()
    local radius = math.random() * 20 + 10
    local life = 0.5
    local dist = math.random() * 30 + 10
    local color = curr_color + vec4(
        math.random() * 0.2 - 0.1,
        math.random() * 0.2 - 0.1,
        math.random() * 0.2 - 0.1, 0)
    color = math.clamp(color, 0, 1)
    local pos = vec3(win:mouse_position(), 0)
    local particle = am.translate(pos) ^ am.circle(vec2(0), radius, color)
    particle:action(am.series{
        am.parallel{
            am.tween(particle"circle", life, {color = color{a=0}}),
            am.tween(particle"translate", life, {position = pos{y = pos.y-dist}}),
        },
        function()
            particles:remove(particle)
            return true
        end})
    particles:append(particle)
end

win.scene = am.postprocess{
        clear_color = vec4(1),
        auto_clear = false
    } ^ am.blend("alpha") ^ particles

win.scene:action(function()
    add_particle()
    if win:mouse_pressed("left") then
        curr_color = vec4(math.random(), math.random(), math.random(), 0.15)
    else
        curr_color = curr_color + vec4(
            (math.random() * 1 - 0.5) * am.delta_time,
            (math.random() * 1 - 0.5) * am.delta_time,
            (math.random() * 1 - 0.5) * am.delta_time,
            0)
    end
    curr_color = math.clamp(curr_color, 0, 1)
end)
