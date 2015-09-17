local devmode = am.devmode

local conf = {} -- global configuration settings

local watched = {}
local watched_objs_mt = {__mode = "k"}

function am.config(obj, index, property)
    if conf[property] then
        obj[index] = conf[property]
    end
    if not devmode then return end
    local watched_objs = watched[property]
    if not watched_objs then
        watched_objs = {}
        setmetatable(watched_objs, watched_objs_mt)
        watched[property] = watched_objs
    end
    local fields = watched_objs[obj]
    if not fields then
        fields = {field}
        watched_objs[obj] = fields
    elseif not table.search(fields, field) then
        table.insert(fields, field)
    end
end

if devmode then
    setmetatable(conf, {__newindex = function(conf, property, value)
        rawset(conf, property, value)
        local watched_objs = watched[property]
        if watched_objs then
            for obj, fields in pairs(watched_objs) do
                for _, field in ipairs(fields) do
                    obj[field] = value
                end
            end
        end
    end})
else
    setmetatable(conf, {__newindex = function(conf, property, value)
       error("must be in dev mode to set configuration properties")
    end})
end
