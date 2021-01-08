local win = am.window{}

local rect = am.rect(-0.5, -0.5, 0.5, 0.5, vec4(0.6, 0.9, 0.1, 1))

local
function rand_color()
    return vec4(math.random(), math.random(), math.random(), 1)
end
rect:action(coroutine.create(function(node)
    while true do
        am.wait(am.tween(node, 0.3, {x1 = -math.random(), color = rand_color()}, am.ease.sine))
        am.wait(am.tween(node, 0.3, {y1 = -math.random(), color = rand_color()}, am.ease.sine))
        am.wait(am.tween(node, 0.3, {x2 = math.random(), color = rand_color()}, am.ease.sine))
        am.wait(am.tween(node, 0.3, {y2 = math.random(), color = rand_color()}, am.ease.sine))
    end
end))

win.scene = am.bind{MV = mat4(1), P = mat4(1)} ^ am.blend("add") ^ rect
win.scene:action(function()
    if win:key_pressed("escape") then
        win:close()
    end
end)
