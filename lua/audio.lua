local root = am.root_audio_node()

am._register_pre_frame_func(function()
    root:remove_all()
end)

function am.schedule_audio(audio_node)
    root:add(audio_node)
end

local buffer_cache = {}

local
function play_file(file, loop, pitch, gain)
    pitch = pitch or 1
    gain = gain or 1
    local buf = buffer_cache[file]
    if not buf then
        buf = am.load_audio(file)
        buffer_cache[file] = buf
    end
    local audio_node = am.track(buf, loop, pitch, gain)
    return am.play(audio_node)
end

local
function play_seed(seed, loop, pitch, gain)
    pitch = pitch or 1
    gain = gain or 1
    local buf = buffer_cache[seed]
    if not buf then
        buf = am.sfxr_synth(seed)
        buffer_cache[seed] = buf
    end
    local audio_node = am.track(buf, loop, pitch, gain)
    return am.play(audio_node)
end

local
function play_buf(buf, loop, pitch, gain)
    pitch = pitch or 1
    gain = gain or 1
    local audio_node = am.track(buf, loop, pitch, gain)
    return am.play(audio_node)
end

function am.play(arg1, arg2, arg3, arg4)
    if not arg1 then
        error("argument 1 is nil", 2)
    end
    local t = am.type(arg1)
    if t == "string" then
        return play_file(arg1, arg2, arg3, arg4)
    elseif t == "number" then
        return play_seed(arg1, arg2, arg3, arg4)
    elseif t == "audio_buffer" then
        return play_buf(arg1, arg2, arg3, arg4)
    else
        local audio_node = arg1
        local keep = arg2
        if keep then
            return function()
                am.schedule_audio(audio_node)
            end
        else
            return function()
                if not audio_node.finished then
                    am.schedule_audio(audio_node)
                else
                    return true
                end
            end
        end
    end
end
