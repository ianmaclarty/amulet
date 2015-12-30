local root = am.root_audio_node()

am._register_pre_frame_func(function()
    root:remove_all()
end)

function am.schedule_audio(audio_node)
    root:add(audio_node)
end

local buffer_cache = {}
setmetatable(buffer_cache, {__mode = "v"})

local
function load_and_play(file, loop)
    local buf = buffer_cache[file]
    if not buf then
        buf = am.load_buffer(file)
        buffer_cache[file] = buf
    end
    local audio_node = am.steam(buffer, loop)
    am.schedule_audio(audio_node)
    return function()
        if not audio_node.finished then
            am.schedule_audio(audio_node)
        else
            return true
        end
    end
end

function am.play(audio_node, keep)
    if type(audio_node) == "string" then
        return load_and_play(audio_node, keep)
    end
    am.schedule_audio(audio_node)
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
