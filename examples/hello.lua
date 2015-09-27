local win = am.window{title = "Untitled", auto_clear = false}

local scene = am.camera2d(200, 150, vec2(0))
    ^ am.scale(vec3(0, 0, 1))
    ^ am.text("Hello, World!", vec4(0.1, 0, 0.05, 0.1))

scene:action(am.tween(scene"scale", 0.8, {scale = vec3(1.5, 1.5, 1)}))

win.scene = scene
