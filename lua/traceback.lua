local remove_c_stack_slots = true

function amulet._traceback(msg)
    msg = debug.traceback(msg, 2)
    if type(msg) == "string" and remove_c_stack_slots then
        msg = msg:gsub("\n\t%[C%]:[^\n]*", ""):gsub("\nstack traceback:$", "")
    end
    return msg:gsub("\t", "    ")
end
