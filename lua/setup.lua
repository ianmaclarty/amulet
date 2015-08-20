local am = amulet

for i, mt in pairs(_metatable_registry) do
    mt._children = {}
end

for i, mt in pairs(_metatable_registry) do
    while mt._parent_mt do
        local parent = mt._parent_mt
        table.insert(parent._children, mt)
        mt = parent
    end
end

local
function set_mt_field_2(mt, field, val)
    for _, child in ipairs(mt._children) do
        if not rawget(child, field) then
            rawset(child, field, val)
        end
        set_mt_field_2(child, field, val)
    end
end

local
function set_mt_field(mt, field, val)
    rawset(mt, field, val)
    set_mt_field_2(mt, field, val)
end

local mtmt = {
    __newindex = set_mt_field,
}
for i, mt in pairs(_metatable_registry) do
    setmetatable(mt, mtmt)
end

local
function define_property(mt, field, getter, setter)
    mt[field] = am._custom_property_marker()
    mt[am._getter_key(field)] = getter
    if setter then
        mt[am._setter_key(field)] = setter
    end
end

local
function create_extension(fields)
    local extension = table.shallow_copy(fields)
    for field, value in pairs(fields) do
        if type(field) ~= "string" then
            error("only string fields allowed for extensions", 3)
        end
        extension[field] = value
        local propname = field:match("^get_([A-Za-z0-9_]+)$")
        if propname and not fields[propname] then
            local getter = value
            local setter = fields["set_"..propname]
            if type(setter) ~= "function" then
                setter = nil
            end
            define_property(extension, propname, getter, setter)
        end
    end
    return extension
end

local
function extend(node, fields)
    node:alias(create_extension(fields))
    return node
end

_metatable_registry.scene_node.extend = extend

local pre_frame_funcs = {}
local post_frame_funcs = {}

function am._pre_frame(dt, curr_time) 
    for i = 1, #pre_frame_funcs do
        pre_frame_funcs[i](dt, curr_time)
    end
end

function am._post_frame(dt, curr_time) 
    for i = 1, #post_frame_funcs do
        post_frame_funcs[i](dt, curr_time)
    end
end

function am._register_pre_frame_func(f)
    table.insert(pre_frame_funcs, f)
end

function am._register_post_frame_func(f)
    table.insert(post_frame_funcs, f)
end
