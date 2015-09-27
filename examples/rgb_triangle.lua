local win = am.window{}

win.scene =
    am.use_program(am.shaders.colors2d)
    ^am.bind{
        P = mat4(1),
        MV = mat4(1),
        color = am.vec4_array{vec4(1, 0, 0, 1), vec4(0, 1, 0, 1), vec4(0, 0, 1, 1)},
        vert = am.vec2_array{vec2(-1, -1), vec2(0, 1), vec2(1, -1)}
    }
    ^am.draw"triangles"
