local win = am.window{}

local capture = am.capture_audio()

local n = 512
local show = 100
local spec_arr = am.buffer(n * 4):view("float", 0, 4)
local spectrum = capture:spectrum(n, spec_arr, 0.5)

local scene = am.group()

scene:action(am.play(spectrum))

local rects = {}
local x = -0.8
local dx = 1.6 / show
local w = 1.0 / show
for i = 1, show do
    rects[i] = am.rect(x, -0.8, x+w, -0.8)
    x = x + dx
    scene:append(rects[i])
end

win.scene = am.bind{MV = mat4(1), P = mat4(1)} ^ scene
scene:action(function()
    if win:key_pressed("escape") then
        win:close()
    end
    for i = 1, show do 
        rects[i].y2 = math.clamp((spec_arr[i] + 50) / 50, -0.8, 0.8)
        rects[i].color = vec4((rects[i].y2 + 0.8)^2, i/show, 1, 1)
    end
end)
