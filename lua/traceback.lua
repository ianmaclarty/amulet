local remove_c_stack_slots = true

function amulet._traceback(msg, thread)
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
    return msg:gsub("\t", "    ")
end
