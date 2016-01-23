win = am.window{title = "Shapes"}

win.scene = am.group{
    -- red rectangle:
    am.rect(-300, 200, -10, 10, vec4(1, 0, 0, 1)),
    -- blue square:
    am.rect(200, -200, 20, -20, vec4(0, 0, 1, 1)),
    -- yellow circle:
    am.circle(vec2(160, 120), 110, vec4(1, 1, 0, 1)),
    -- green hexagon:
    am.circle(vec2(-160, -120), 110, vec4(0, 1, 0, 1), 6),
}
