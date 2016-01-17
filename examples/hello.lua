local win = am.window{title = "Hello"}

local scene =
    am.scale(0)
    ^ am.text("Hello, World!", "center", "center")

scene:action(am.tween(scene"scale", 0.8, {scale2d = vec2(4)}))

win.scene = scene
