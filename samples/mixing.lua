local am = amulet

local win = am.window{}

local tracks = {}

for i = 1, 6 do
    tracks[i] = am.stream(am.read_buffer("track"..i..".ogg"), true)
end

local master = am.audio_node()

local is_playing = false
local
function play_pause()
    if is_playing then
        am.root_audio_node():remove(master)
    else
        am.root_audio_node():add(master)
    end
    is_playing = not is_playing
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

win.root = am.group()
win.root:action(function()
    if win:key_pressed("enter") then
        play_pause()
    elseif win:key_pressed("r") then
        select_tracks()
    elseif win:key_pressed("escape") then
        win:close()
    end
end)
