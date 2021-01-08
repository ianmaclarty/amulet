win = am.window{title = "Keyboard"}

img = [[
..W..
WWWWW
W.W.W
..W..
WWWWW
W...W
W...W
]]

person = am.translate(0, 0) ^ am.scale(8) ^ am.sprite(img)

x, y = 0, 0

person:action(function()
    local dt = am.delta_time
    if win:key_down"left" then
        x = x - 100 * dt
    elseif win:key_down"right" then
        x = x + 100 * dt
    end
    if win:key_down"down" then
        y = y - 100 * dt
    elseif win:key_down"up" then
        y = y + 100 * dt
    end
    person.position2d = vec2(x, y)

    if win:key_pressed"x" then
        person"sprite".color = vec4(
            math.random(),
            math.random(),
            math.random(), 1)
    end
end)

text = am.text("PRESS ARROW KEYS TO MOVE\nPRESS X TO CHANGE COLOUR")

win.scene = am.group{
    person,
    am.translate(0, -200) ^ text
}
