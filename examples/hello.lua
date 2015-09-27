local win = am.window{title = "Untitled", clear_color = vec4(1, 0, 0.5, 1)}

local scene = am.camera2d(200, 150, vec2(0))
    ^ am.scale(vec3(0, 0, 1))
    ^ am.text("Hello, World!")

scene:action(am.tween(scene"scale", 0.8, {scale = vec3(1, 1, 1)}))

win.scene = scene
