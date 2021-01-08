local win = am.window{}

local tracks = {}

for i = 1, 6 do
    tracks[i] = am.stream(am.load_buffer("track"..i..".ogg"), true)
end

local master = am.audio_node()

local n = 512
local show = 100
local spec_arr = am.buffer(n * 4):view("float", 0, 4)
local spectrum = master:spectrum(n, spec_arr, 0.5)

local scene = am.group()
local rects = {}
local x = -0.8
local dx = 1.6 / show
local w = 1.0 / show
for i = 1, show do
    rects[i] = am.rect(x, -0.8, x+w, -0.8)
    x = x + dx
    scene:append(rects[i])
end

local
function toggle_pause()
    master.paused = not master.paused
end

local
function select_tracks()
    master:remove_all()
    repeat
        local n = 0
        for i = 1, #tracks do
            if math.random(2) == 1 then
                master:add(tracks[i])
                n = n + 1
            end
        end
    until n > 0
end

select_tracks()

scene:action(am.play(spectrum))

win.scene = am.bind{MV = mat4(1), P = mat4(1)} ^ scene
scene:action(function()
    if win:key_pressed("enter") then
        toggle_pause()
    elseif win:key_pressed("r") then
        select_tracks()
    elseif win:key_pressed("escape") then
        win:close()
    end
    for i = 1, show do 
        rects[i].y2 = math.clamp((spec_arr[i] + 50) / 50, -0.8, 0.8)
        rects[i].color = vec4((rects[i].y2 + 0.8)^2, i/show, 1, 1)
    end
end)
