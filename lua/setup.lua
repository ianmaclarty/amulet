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

local iap_transaction_callback
local iap_restore_callback
local iap_retrieve_callback

function am._iap_transaction_updated(productid, status)
    if not iap_transaction_callback then
        error("no iap transaction callback registered!")
    else
        return iap_transaction_callback(productid, status)
    end
end

function am.register_iap_transaction_callback(cb)
    iap_transaction_callback = cb
end

function am._iap_restore_finished(success)
    if iap_restore_callback then
        iap_restore_callback(success)
    end
end

function am.register_iap_restore_callback(cb)
    iap_restore_callback = cb
end

function am._iap_retrieve_products_finished(products)
    if iap_retrieve_callback then
        iap_retrieve_callback(products)
    end
end

function am.register_iap_retrieve_callback(cb)
    iap_retrieve_callback = cb
end
