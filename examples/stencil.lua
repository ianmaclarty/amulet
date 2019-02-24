local win = am.window{
    stencil_buffer = true,
}

local scene = 
    am.stencil_test{
        enabled = true,
        ref = 0,
        read_mask = 1,
        write_mask = 1,
        func_front = "equal",
        op_fail_front = "invert",
        op_zfail_front = "invert",
        op_zpass_front = "invert",
    } ^
    am.group{
        am.rect(-300, -100, 100, 100, vec4(1, 0.5, 0, 1)),
        am.rect(0, -50, 300, 50, vec4(1, 0, 0, 1)),
        am.rect(-100, -300, 120, 300, vec4(0, 0.5, 1, 1)),
        am.circle(vec2(150, 0), 200),
    }

win.scene = scene

scene:action(function()
    scene.ref = math.floor(math.fract(am.frame_time) + 0.5)
end)
