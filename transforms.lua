win = am.window{title = "Transforms",
    clear_color = vec4(0.8, 0.7, 0.4, 1)}

img = [[
....ggggg.....
.gggggggggg...
.ggggggggggg..
gggggggggggggg
ggggggggggggg.
.ggggggggggggg
....gggggg....
......oo......
......oo......
......oo......
......oo......
]]

sprite = am.sprite(img)

win.scene = am.group{
    am.translate(-100, 100) ^ am.scale(10) ^ sprite,
    am.translate(-100, -100) ^ am.rotate(math.rad(45)) ^ am.scale(5) ^ sprite,
    am.translate(100, 100) ^ am.scale(10, 3) ^ sprite,
    am.translate(100, -100) ^ am.scale(-5, 15) ^ am.rotate(math.rad(30)) ^ sprite,
}
