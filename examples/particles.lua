local win = am.window{
    clear_color = vec4(0.1, 0.03, 0.01, 1),
    letterbox = false,
}

win.scene = am.blend("add_alpha")
    ^ am.particles2d{
        source_pos = win:mouse_position(),
        source_pos_var = vec2(20),
        max_particles = 1000,
        emission_rate = 500,
        start_particles = 0,
        life = 0.4,
        life_var = 0.1,
        angle = math.rad(90),
        angle_var = math.rad(180),
        speed = 200,
        start_color = vec4(1, 0.3, 0.01, 0.5),
        start_color_var = vec4(0.1, 0.05, 0.0, 0.1),
        end_color = vec4(0.5, 0.8, 1, 1),
        end_color_var = vec4(0.1),
        start_size = 30,
        start_size_var = 10,
        end_size = 2,
        end_size_var = 2,
        gravity = vec2(0, 2000),
    }
    :action(function(node)
        node.source_pos = win:mouse_position()
    end)

win.scene:action(am.tween(win.scene"particles2d", 10, {gravity = vec2(0, -2000)}))
