-- multiple windows do not yet work properly on all platforms

local window1 = am.window{title="Window 1"}
local window2 = am.window{title="Window 2"}
window1.scene = am.rotate(0) ^ am.scale(4) ^ am.text("Window 1", vec4(1, 0, 0.5, 1))
window2.scene = am.rotate(0) ^ am.scale(4) ^ am.text("Window 2", vec4(0, 1, 1, 1))

window1.scene:action(function(node)
    node.angle = am.frame_time * math.pi * 2
    if window1:key_pressed"1" then
        window1:close()
    end
    if window1:mouse_pressed"left" then
        window1.clear_color = math.randvec4()
    end
end)
local t = 0
window2.scene:action(function(node)
    t = t + am.delta_time
    node.angle = t * math.pi * 2
    if window2:key_pressed"2" then
        window2:close()
    end
    if window2:mouse_pressed"left" then
        window2.clear_color = math.randvec4()
    end
end)
