function am.float_array(values)
    local view = am.buffer(4 * #values):view("float", 0, 4)
    view:set(values)
    return view
end

function am.byte_array(values)
    local view = am.buffer(#values):view("byte", 0, 1)
    view:set(values)
    return view
end

function am.ubyte_array(values)
    local view = am.buffer(#values):view("ubyte", 0, 1)
    view:set(values)
    return view
end

function am.byte_norm_array(values)
    local view = am.buffer(#values):view("byte_norm", 0, 1)
    view:set(values)
    return view
end

function am.ubyte_norm_array(values)
    local view = am.buffer(#values):view("ubyte_norm", 0, 1)
    view:set(values)
    return view
end

function am.short_array(values)
    local view = am.buffer(#values):view("short", 0, 2)
    view:set(values)
    return view
end

function am.ushort_array(values)
    local view = am.buffer(#values):view("ushort", 0, 2)
    view:set(values)
    return view
end

function am.short_norm_array(values)
    local view = am.buffer(#values):view("short_norm", 0, 2)
    view:set(values)
    return view
end

function am.ushort_norm_array(values)
    local view = am.buffer(#values):view("ushort_norm", 0, 2)
    view:set(values)
    return view
end

function am.int_array(values)
    local view = am.buffer(#values):view("int", 0, 4)
    view:set(values)
    return view
end

function am.uint_array(values)
    local view = am.buffer(#values):view("uint", 0, 4)
    view:set(values)
    return view
end

function am.int_norm_array(values)
    local view = am.buffer(#values):view("int_norm", 0, 4)
    view:set(values)
    return view
end

function am.uint_norm_array(values)
    local view = am.buffer(#values):view("uint_norm", 0, 4)
    view:set(values)
    return view
end

function am.ushort_elem_array(values)
    local view = am.buffer(2 * #values):view("ushort_elem", 0, 2)
    view:set(values)
    return view
end

function am.uint_elem_array(values)
    local view = am.buffer(4 * #values):view("uint_elem", 0, 4)
    view:set(values)
    return view
end

local
function vec_array(values, name, components)
    local stride = components * 4
    local len = #values
    if len == 0 then
        return am.buffer(0):view(name, 0, stride)
    elseif type(values[1]) == "number" then
        if len % components ~= 0 then
            error("number of elements should be divisible by "..components, 2)
        end
        len = len / components
    end
    local view = am.buffer(len * stride):view(name, 0, stride)
    view:set(values)
    return view
end

function am.vec2_array(values)
    return vec_array(values, "vec2", 2)
end

function am.vec3_array(values)
    return vec_array(values, "vec3", 3)
end

function am.vec4_array(values)
    return vec_array(values, "vec4", 4)
end
