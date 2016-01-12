function am.save_state(name, state)
    local str = table.tostring(state)
    if am.platform == "html" then
        am.eval_js("localStorage.setItem('"..name.."','return "..
            str:gsub("%'", "\\'"):gsub("%\n", "\\n").."');")
    else
        local f = io.open(am.app_data_dir..name..".sav", "w")
        f:write("return ")
        f:write(str)
        f:close()
    end
end

function am.load_state(name)
    local str
    if am.platform == "html" then
        str = am.eval_js("localStorage.getItem('"..name.."');")
        if not str then
            return nil
        end
    else
        local f = io.open(am.app_data_dir..name..".sav", "r")
        if not f then
            return nil
        end
        str = f:read("*a")
        f:close()
    end
    return loadstring(str)()
end
