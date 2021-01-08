win = am.window{title = "Text"}

win.scene = am.group{
    -- background grey rectables for reference:
    am.rect(-100, -240, 100, 240, vec4(0.2, 0.2, 0.2, 1)),
    am.rect(-320, -100, 320, 100, vec4(0.2, 0.2, 0.2, 1)),
    am.rect(-100, -100, 100, 100, vec4(0.3, 0.3, 0.3, 1)),

    -- text nodes:
    am.translate(-100, 100)
    ^ am.scale(2)
    ^ am.text("Centered")
    ,
    am.translate(100, 100)
    ^ am.text("Left\nCenter\nAligned\nText", vec4(0, 1, 0.5, 1), "left")
    ,
    am.translate(-100, -100)
    ^ am.text("Right\nBottom\nAligned\nText", vec4(1, 0, 0.5, 1), "right", "bottom")
    ,
    am.translate(100, -100)
    ^ am.text("Left\nTop\nAligned\nText", vec4(1, 0.5, 0, 1), "left", "top")
    ,
    am.text([[
╔═══════════╗
║ ♥ UTF-8 ♦ ║
║ ♫ Text‼ ☺ ║
╚═══════════╝]])
}
