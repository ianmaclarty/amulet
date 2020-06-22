local debug = _G.debug

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

    local is_desktop = am.platform == "windows" or am.platform == "osx" or am.platform == "linux"
    local logfile = am.app_data_dir.."log.txt"

    if am._main_window then
        local txt = "ERROR:\n\n"..msg.."\n\n"
        if am.support_email then
            txt = txt.."\nPlease send a screenshot of this error to "..am.support_email..".\n"
        end
        if is_desktop then
            txt = txt.."\nA copy of this error has been saved to "..logfile..".\n\nPress ESC to close the application.\n"
        end
        pcall(function()
            local win = am._main_window
            local w, h = win.pixel_width, win.pixel_height
            win.clear_color = vec4(0, 0, 1, 1)
            win.scene = am.bind{P = math.ortho(0, w, 0, h, -1, 1)}
                ^ am.translate(10, h - 10)
                ^ am.text(txt, vec4(1, 1, 1, 1), "left", "top")
            win.scene:action(function()
                if win:key_pressed"escape" then
                    win:close()
                end
            end)
        end)
    end

    -- write stacktrace to log file
    local plat = am.platform
    if is_desktop then
        local f = io.open(logfile, "w")
        if f then
            f:write(msg)
            f:write("\n")
            f:close()
        end
    end

    return msg
end
