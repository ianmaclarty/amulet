local remove_c_stack_slots = true

function am._traceback(msg, thread)
    local match = msg:match("^.*__coroutine__(.*)$")
    if match then
        return match
    end
    if thread then
        msg = debug.traceback(thread, msg, 2)
    else
        msg = debug.traceback(msg, 2)
    end
    if type(msg) == "string" and remove_c_stack_slots then
        msg = msg:gsub("\n\t%[C%]:[^\n]*", ""):gsub("\nstack traceback:$", "")
    end
    local msg = msg:gsub("\t", "    ")
    if am._main_window then
        pcall(function()
            local win = am._main_window
            local w, h = win.pixel_width, win.pixel_height
            win.clear_color = vec4(0, 0, 1, 1)
            win.scene = am.bind{P = math.ortho(0, w, 0, h, -1, 1)}
                ^ am.translate(10, h - 10)
                ^ am.text("ERROR:\n"..msg, vec4(1, 1, 1, 1), "left", "top")
        end)
    end
    return msg
end
