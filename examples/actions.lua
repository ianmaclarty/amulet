win = am.window{title = "Actions"}

img = [[
YYYYYYY
YCCCCCY
YCMMMCY
YCMGMCY
YCMMMCY
YCCCCCY
YYYYYYY
]]

squares = am.translate(-120, 0) ^ am.rotate(0) ^ am.scale(10) ^ am.sprite(img)

ball = am.translate(120, 100) ^ am.scale(1) ^ am.circle(vec2(0), 50)

group = am.group()
group:append(squares)
group:append(ball)

group:action(function()
    local t = am.frame_time
    squares"rotate".angle = t
    squares"scale".scale2d = vec2(math.sin(t) * 5 + 8)
    squares"sprite".color = vec4(math.fract(vec3(t, t * 3, t * 7)), 1)
    ball"circle".color = vec4(
        math.sin(t) * 0.5 + 0.5,
        math.cos(t * 5) * 0.5 + 0.5,
        math.sin(t * 13) * 0.5 + 0.5,
    1)
    ball"translate".position2d = vec2(120, math.abs(math.cos(t)) * 300 - 200)
end)

win.scene = group
