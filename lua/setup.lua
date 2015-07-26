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

local extensions = {}
local max_extensions = 1000

local
function create_extension(obj, methods)
    local obj_mt = getmetatable(obj)
    local ext_mt = table.shallow_copy(obj_mt)
    table.insert(obj_mt._children, ext_mt)
    rawset(ext_mt, "_children", {})
    setmetatable(ext_mt, mtmt)
    rawset(ext_mt, "_target_tname", obj.tname)
    for field, func in pairs(methods) do
        if type(field) ~= "string" then
            error("method names must be strings", 3)
        elseif type(func) ~= "function" then
            error("method "..field.." is not a function", 3)
        end
        ext_mt[field] = func
        local propname = field:match("^get_([A-Za-z0-9_]+)$")
        if propname then
            local getter = func
            local setter = methods["set_"..propname]
            if type(setter) ~= "function" then
                setter = nil
            end
            define_property(ext_mt, propname, getter, setter)
        end
    end
    return ext_mt
end

local
function extend(obj, methods)
    local ext = extensions[methods]
    if not ext then
        if #extensions > max_extensions then
            error("attempt to define more than "..max_extensions.." extensions", 2)
        end
        ext = create_extension(obj, methods)
        extensions[methods] = ext
    elseif ext._target_tname ~= obj.tname then
        error("attempt to use 1 extension table with 2 object types (first "
            ..ext._target_tname.." and now "..obj.tname..")", 2)
    end
    am._setmetatable(obj, ext)
    return obj
end

_metatable_registry.scene_node.extend = extend
