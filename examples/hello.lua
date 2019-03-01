local win = am.window{title = "Hello"}

local scene =
    am.scale(4)
    ^ am.text("Hello, World!", "center", "center")

local i = 0
scene:action(function()
    --if win:key_pressed"enter" then
        i = i + 1
        scene"text".text = "test"..i
    --end
end)

win.scene = scene
