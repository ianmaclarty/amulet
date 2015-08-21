local am = amulet
local root = am.root_audio_node()

am._register_pre_frame_func(function()
    root:remove_all()
end)

function am.play(audio_node)
    return function()
        if not audio_node.finished then
            root:add(audio_node)
        else
            return true
        end
    end
end
