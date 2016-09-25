function am.save_state(name, state, format)
    format = format or "lua"
    local str
    if format == "lua" then
        str = "return "..table.tostring(state)
    elseif format == "json" then
        str = am.to_json(state)
    else
        error("unknown format: "..format, 2)
    end
    if am.platform == "html" then
        am.eval_js("localStorage.setItem('"..name.."','"..
            str:gsub("%'", "\\'"):gsub("%\n", "\\n").."');")
    elseif am.platform == "ios" then
        am.ios_store_pref(name, str)
    else
        local f = io.open(am.app_data_dir..name..".sav", "w")
        f:write(str)
        f:close()
    end
end

function am.load_state(name, format)
    format = format or "lua"
    local str
    if am.platform == "html" then
        str = am.eval_js("localStorage.getItem('"..name.."');")
        if not str then
            return nil
        end
    elseif am.platform == "ios" then
        str = am.ios_retrieve_pref(name)
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
    if format == "lua" then
        return loadstring(str)()
    elseif format == "json" then
        local res, err = am.parse_json(str)
        if res == nil and err then
            error("error parsing JSON while loading "..name..":\n"..err, 2)
        else
            return res
        end
    else
        error("unknown format: "..format, 2)
    end
end
