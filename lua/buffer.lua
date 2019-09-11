local view_type_info = {
    float = {size = 4, components = 1},
    vec2  = {size = 4, components = 2},
    vec3  = {size = 4, components = 3},
    vec4  = {size = 4, components = 4},
    mat3  = {size = 4, components = 9},
    mat4  = {size = 4, components = 16},

    double = {size = 8, components = 1},
    dvec2  = {size = 8, components = 2},
    dvec3  = {size = 8, components = 3},
    dvec4  = {size = 8, components = 4},
    dmat3  = {size = 8, components = 9},
    dmat4  = {size = 8, components = 16},

    byte  = {size = 1, components = 1},
    byte2 = {size = 1, components = 2},
    byte3 = {size = 1, components = 3},
    byte4 = {size = 1, components = 4},

    ubyte  = {size = 1, components = 1},
    ubyte2 = {size = 1, components = 2},
    ubyte3 = {size = 1, components = 3},
    ubyte4 = {size = 1, components = 4},

    byte_norm  = {size = 1, components = 1},
    byte_norm2 = {size = 1, components = 2},
    byte_norm3 = {size = 1, components = 3},
    byte_norm4 = {size = 1, components = 4},

    ubyte_norm  = {size = 1, components = 1},
    ubyte_norm2 = {size = 1, components = 2},
    ubyte_norm3 = {size = 1, components = 3},
    ubyte_norm4 = {size = 1, components = 4},

    short  = {size = 2, components = 1},
    short2 = {size = 2, components = 2},
    short3 = {size = 2, components = 3},
    short4 = {size = 2, components = 4},

    ushort  = {size = 2, components = 1},
    ushort2 = {size = 2, components = 2},
    ushort3 = {size = 2, components = 3},
    ushort4 = {size = 2, components = 4},

    short_norm  = {size = 2, components = 1},
    short_norm2 = {size = 2, components = 2},
    short_norm3 = {size = 2, components = 3},
    short_norm4 = {size = 2, components = 4},

    ushort_norm  = {size = 2, components = 1},
    ushort_norm2 = {size = 2, components = 2},
    ushort_norm3 = {size = 2, components = 3},
    ushort_norm4 = {size = 2, components = 4},

    ushort_elem = {size = 2, components = 1},

    int  = {size = 4, components = 1},
    int2 = {size = 4, components = 2},
    int3 = {size = 4, components = 3},
    int4 = {size = 4, components = 4},

    uint  = {size = 4, components = 1},
    uint2 = {size = 4, components = 2},
    uint3 = {size = 4, components = 3},
    uint4 = {size = 4, components = 4},

    uint_elem = {size = 4, components = 1},
}

function am.float_array(values)
    if type(values) == "table" then
        local view = am.buffer(4 * #values):view("float", 0, 4)
        view:set(values)
        return view
    else
        local view = am.buffer(4 * values):view("float", 0, 4)
        return view
    end
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

function mathv.array(elemtype, len, init)
    local data = nil
    local info = view_type_info[elemtype]
    if not info then
        error("unknown view element type: "..tostring(elemtype), 2)
    end
    local components = info.components
    local elemsize = info.size
    local stride = components * elemsize
    if type(len) == "table" then
        data = len
        len = #data
        if len == 0 then
            return am.buffer(0):view(elemtype)
        end
        if type(data[1]) == "number" then
            if len % components ~= 0 then
                error("number of data elements should be divisible by "..components, 2)
            end
            len = len / components
        end
        local view = am.buffer(len * stride):view(elemtype)
        view:set(data)
        return view
    else
        local view = am.buffer(len * stride):view(elemtype)
        if init then
            view:set(init)
        end
        return view
    end
end

function mathv.cast(elemtype, arr)
    local info = view_type_info[elemtype]
    if not info then
        error("unknown view element type: "..tostring(elemtype), 2)
    end
    local components = info.components
    local elemsize = info.size
    local stride = components * elemsize
    local len = #arr
    local view = am.buffer(len * stride):view(elemtype)
    view:set(arr)
    return view
end

local view_group_mt
view_group_mt = {
    __index = function(struct, field)
        local view = struct._views[field]
        if view then
            return view
        end
        local method = view_group_mt[field]
        if method then
            return method
        end
        return nil
    end,
    __newindex = function(struct, field, val)
        struct._views[field]:set(val)
    end,
    filter = function(struct, pred)
        local n = 0
        for key, val in pairs(struct._views) do
            n = val:filter(pred)
        end
        return n
    end,
    slice = function(struct, start, count, step)
        local result = {_views = {}}
        for key, val in pairs(struct._views) do
            result._views[key] = val:slice(start, count, step)
        end
        setmetatable(result, view_group_mt)
        return result
    end,
}

local
function create_array_of_structs(capacity, spec)
    local n = #spec
    if n % 2 ~= 0 then
        error("each attribute must have a corresponding type", 3)
    elseif n == 0 then
        error("no attributes given", 3)
    end
    local attrs = {}
    local stride = 0
    local align = 4
    for i = 1, n, 2 do
        local attr = spec[i]
        local tp = spec[i + 1]
        local info = view_type_info[tp]
        if not info then
            error("unknown view element type: "..tostring(tp), 3)
        end
        local sz = info.size * info.components
        -- align field
        while stride % info.size ~= 0 do
            stride = stride + 1
        end
        align = math.max(info.size, align)
        attrs[attr] = {type = tp, offset = stride}
        stride = stride + sz
    end
    -- align struct
    while stride % align ~= 0 do
        stride = stride + 1
    end
    local buffer = am.buffer(capacity * stride)
    local views = {}
    for attr, info in pairs(attrs) do
        views[attr] = buffer:view(info.type, info.offset, stride)
    end
    return views
end

local
function create_struct_of_arrays(capacity, spec)
    local n = #spec
    if n % 2 ~= 0 then
        error("each attribute must have a corresponding type", 3)
    elseif n == 0 then
        error("no attributes given", 3)
    end
    local offset = 0
    local attrs = {}
    for i = 1, n, 2 do
        local attr = spec[i]
        local tp = spec[i + 1]
        local info = view_type_info[tp]
        if not info then
            error("unknown view element type: "..tostring(tp), 3)
        end
        local stride = info.size * info.components
        attrs[attr] = {
            offset = offset,
            type = tp,
            stride = stride,
        }
        offset = offset + capacity * stride
        -- 8-byte align each view
        while offset % 8 ~= 0 do
            offset = offset + 1
        end
    end
    local buffer = am.buffer(offset)
    local views = {}
    for attr, info in pairs(attrs) do
        views[attr] = buffer:view(info.type, info.offset, info.stride, capacity)
    end
    return views
end

function mathv.view_group(views)
    local group = {
        _views = views,
    }
    setmetatable(group, view_group_mt)
    return group
end

function mathv.array_of_structs(capacity, spec)
    return mathv.view_group(create_array_of_structs(capacity, spec))
end

function mathv.struct_of_arrays(capacity, spec)
    return mathv.view_group(create_struct_of_arrays(capacity, spec))
end

-- legacy function (keep for backwards compatibility)
function am.struct_array(capacity, spec)
    return create_array_of_structs(capacity, spec)
end
