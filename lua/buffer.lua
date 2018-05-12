local view_type_size = {
    float = 4,
    vec2 = 8,
    vec3 = 12,
    vec4 = 16,
    byte = 1,
    ubyte = 1,
    byte_norm = 1,
    ubyte_norm = 1,
    short = 2,
    ushort = 2,
    short_norm = 2,
    ushort_norm = 2,
    ushort_elem = 2,
    int = 4,
    uint = 4,
    uint_elem = 4,
}

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
    if type(values) == "table" then
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
    else
        -- values is length
        return am.buffer(values * stride):view(name, 0, stride)
    end
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

local struct_array_mt = {}

function am.struct_array(capacity, spec)
    local n = #spec
    if n % 2 ~= 0 then
        error("each attribute must have a corresponding type", 2)
    elseif n == 0 then
        error("no attributes given", 2)
    end
    local attrs = {}
    local stride = 0
    for i = 1, n, 2 do
        local attr = spec[i]
        local tp = spec[i + 1]
        local sz = view_type_size[tp]
        if not sz then
            error("unknown view element type: "..tostring(tp), 2)
        end
        -- align
        while stride % math.min(4, sz) ~= 0 do
            stride = stride + 1
        end
        attrs[attr] = {type = tp, offset = stride}
        stride = stride + sz
    end
    -- 4 byte align
    while stride % 4 ~= 0 do
        stride = stride + 1
    end
    local buffer = am.buffer(capacity * stride)
    local views = {}
    for attr, info in pairs(attrs) do
        views[attr] = buffer:view(info.type, info.offset, stride)
    end
    return views
end
