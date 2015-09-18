local win = 
am.window{
    top = 0,
    left = 0,
    mode = "fullscreen"
    }

win.root =
    am.use_program(am.shaders.colors2d)
    :action(function()
        if win:key_pressed("escape") then
            win:close()
        end
    end)
    ^am.bind{
        P = mat4(1),
        MV = mat4(1),
        color = am.vec4_array{vec4(1, 0, 0, 1), vec4(0, 1, 0, 1), vec4(0, 0, 1, 1)},
        vert = am.vec2_array{vec2(-1, -1), vec2(0, 1), vec2(1, -1)}
    }
    ^am.draw"triangles"
