local root = am.root_audio_node()

am._register_pre_frame_func(function()
    root:remove_all()
end)

function am.schedule_audio(audio_node)
    root:add(audio_node)
end

function am.play(audio_node, keep)
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
