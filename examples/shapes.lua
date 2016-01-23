win = am.window{title = "Shapes"}

win.scene = am.group{
    -- red rectangle:
    am.rect(-250, 180, -60, 30, vec4(1, 0, 0, 1)),
    -- yellow circle:
    am.circle(vec2(160, 120), 110, vec4(1, 1, 0, 1)),
    -- blue triangle:
    am.circle(vec2(160, -120), 110, vec4(0, 0, 1, 1), 3),
    -- green hexagon:
    am.circle(vec2(-160, -120), 110, vec4(0, 1, 0, 1), 6),
}
