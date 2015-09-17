local get_mt = getmetatable

function am.type(val)
    local t = type(val)
    if t == "userdata" then
        local mt = get_mt(val) 
        if mt then
            return mt.tname or t
        else
            return t
        end
    else
        return t
    end
end
