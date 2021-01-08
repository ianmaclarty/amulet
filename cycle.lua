local win = am.window{}

local rect = am.rect(-200, -200, 200, 200, vec4(1, 0.2, 0.1, 0.1))
rect"blend".mode = "add_alpha"
local cycle = am.scale(0.95) ^ am.rotate(math.pi/10) ^ rect
cycle"rotate":append(cycle)
cycle.recursion_limit = 50
cycle"rotate".recursion_limit = 50

win.scene = cycle
win.scene:action(function()
    cycle"rotate".angle = am.frame_time / 10
end)
