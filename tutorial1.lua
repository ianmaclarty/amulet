local red = vec4(1, 0, 0, 1)
local blue = vec4(0, 0, 1, 1)
local yellow = vec4(1, 1, 0, 1)

local win = am.window{
    title = "Hi",
    width = 400,
    height = 300,
    clear_color = blue,
}

win.scene =
    am.group()
    ^ {
        am.translate(-150, 0):tag"left_eye"
        ^ am.circle(vec2(0, 0), 50, red)
        ,
        am.translate(150, 0):tag"right_eye"
        ^ am.circle(vec2(0, 0), 50, red)
        ,
        am.translate(0, -25)
        ^ am.rect(-50, -50, 50, 50, yellow)
    }

win.scene:action(function(scene)
    scene"left_eye".hidden = not win:key_down"a"
    scene"right_eye".hidden = not win:key_down"b"
end)

win.scene"left_eye".hidden = true
win.scene"right_eye".hidden = true
