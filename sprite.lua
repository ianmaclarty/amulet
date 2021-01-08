win = am.window{title = "Sprite",
    width = 100, height = 100,
    clear_color = vec4(0.6, 0.8, 1, 1)
}

img = [[
...cc.ccc.
.cccccccc.
.cYYcYYYc.
YYgWYYgWcY
YYYYYYYYYY
.YRYYYYRY.
.YYRRRRY..
....YY....
.MmMYYmMm.
.MmMmMmMm.
]]

win.scene = am.scale(10) ^ am.sprite(img)
