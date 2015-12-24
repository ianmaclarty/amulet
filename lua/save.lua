function am.save_state(name, state)
    local str = table.tostring(state)
    local f = io.open(am.app_data_dir..name..".sav", "w")
    f:write("return ")
    f:write(str)
    f:close()
end

function am.load_state(name)
    local f = io.open(am.app_data_dir..name..".sav", "r")
    if not f then
        return nil
    end
    local str = f:read("*a")
    f:close()
    return loadstring(str)()
end
