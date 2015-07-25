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
function set_mt_field(mt, field, val)
    rawset(mt, field, val)
    for _, child in ipairs(mt._children) do
        set_mt_field(child, field, val)
    end
end

for i, mt in pairs(_metatable_registry) do
    local mtmt = {
        __newindex = set_mt_field
    }
    setmetatable(mt, mtmt)
end
