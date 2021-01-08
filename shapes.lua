win = am.window{title = "Shapes"}

win.scene = am.group{
    -- red rectangle:
    am.rect(-250, 180, -80, 30, vec4(1, 0, 0, 1)),
    -- yellow circle:
    am.circle(vec2(160, 120), 80, vec4(1, 1, 0, 1)),
    -- magenta line:
    am.line(vec2(-20, -100), vec2(20, 100), 10, vec4(1, 0, 1, 1)),
    -- blue triangle:
    am.circle(vec2(160, -120), 80, vec4(0, 0, 1, 1), 3),
    -- green hexagon:
    am.circle(vec2(-160, -120), 80, vec4(0, 1, 0, 1), 6),
}
