-- extra table functions

function table.shallow_copy(t)
    if type(t) ~= "table" then
        error("table expected, but got a "..type(t), 2)
    end
    local copy = {}
    for k, v in pairs(t) do
        copy[k] = v
    end
    return copy
end

local
function deep_copy_2(t, seen)
    local s = seen[t]
    if s then
        return s
    else
        s = {}
        seen[t] = s
        for k, v in pairs(t) do
            if type(k) == "table" then
                k = deep_copy_2(k, seen)
            elseif type(k) == "userdata" then
                error("cannot deep copy userdata values", 3)
            end
            if type(v) == "table" then
                v = deep_copy_2(v, seen)
            elseif type(v) == "userdata" then
                if type(k) == "string" then
                    error("cannot copy userdata value '"..k.."'", 3)
                else
                    error("cannot deep copy userdata values", 3)
                end
            end
            s[k] = v
        end
        return s
    end
end

function table.deep_copy(t)
    if type(t) == "table" then
        return deep_copy_2(t, {})
    else
        error("table expected, but got a "..type(t), 2)
    end
end

function table.search(t, elem)
    for i = 1, #t do
        if t[i] == elem then
            return i
        end
    end
    return nil
end

function table.append(arr1, arr2)
    local i = #arr1 + 1
    for _, v in ipairs(arr2) do
        arr1[i] = v
        i = i + 1
    end
end

function table.merge(t1, t2)
    for k, v in pairs(t2) do
        t1[k] = v
    end
end

function table.keys(t)
    local ks = {}
    local i = 1
    for k, _ in pairs(t) do
        ks[i] = k
        i = i + 1
    end
    return ks
end

function table.values(t)
    local vs = {}
    local i = 1
    for _, v in pairs(t) do
        vs[i] = v
        i = i + 1
    end
    return vs
end

function table.filter(t, f)
    local t2 = {}
    local i = 1
    for _, v in ipairs(t) do
        if f(v) then
            t2[i] = v
            i = i + 1
        end
    end
    return t2
end

function table.clear(t)
    for k, _ in pairs(t) do
        t[k] = nil
    end
end

local
function table_tostring(t, indent)
    local tp = type(t)
    if tp == "table" then
        indent = indent or 4
        local tab = "    "
        local prefix = string.rep(tab, indent)
        local str = "{\n"
        for key, value in pairs(t) do
            local keystr
            if type(key) == "string" then
                keystr = key
            else
                keystr = "[" .. tostring(key) .. "]"
            end
            str = str .. prefix .. tab .. keystr .. " = " .. table_tostring(value, indent + 1) .. ",\n"
        end
        str = str .. prefix .. "}"
        return str
    elseif tp == "string" then
        return '"' .. t:gsub("\"", "\\\"") .. '"'
    else
        return tostring(t)
    end
end

table.tostring = table_tostring

-- override some maths functions to avoid nans

local acos = math.acos
local asin = math.asin
local clamp = math.clamp

function math.acos(x)
    return acos(math.clamp(x, -1, 1))
end

function math.asin(x)
    return asin(math.clamp(x, -1, 1))
end

-- vector/matrix stuff

rawset(_G, "vec2", math.vec2)
rawset(_G, "vec3", math.vec3)
rawset(_G, "vec4", math.vec4)
rawset(_G, "mat2", math.mat2)
rawset(_G, "mat3", math.mat3)
rawset(_G, "mat4", math.mat4)
rawset(_G, "quat", math.quat)

local
function format_vec(v)
    local n = #v
    local str = "vec"..n.."("
    for i = 1, n do
        str = str..string.format("%g", v[i])
        if i == n then
            str = str..")"
        else
            str = str..", "
        end
    end
    return str
end

_metatable_registry.vec2.__tostring = format_vec
_metatable_registry.vec3.__tostring = format_vec
_metatable_registry.vec4.__tostring = format_vec

local
function format_mat(m)
    local n = #m
    local rowwidth = {0, 0, 0, 0}
    for col = 1, n do
        for row = 1, n do
            local str = tostring(m[col][row])
            if #str > rowwidth[row] then
                rowwidth[row] = #str
            end
        end
    end
    local str = "mat"..n.."("
    for col = 1, n do
        for row = 1, n do
            local fmt = "%"..rowwidth[row].."s"
            str = str..string.format(fmt, tostring(m[col][row]))
            if row ~= n then
                str = str..", "
            end
        end
        if col == n then
            str = str..")"
        else
            str = str..",\n     "
        end
    end
    return str
end

_metatable_registry.mat2.__tostring = format_mat
_metatable_registry.mat3.__tostring = format_mat
_metatable_registry.mat4.__tostring = format_mat

local
function format_quat(q)
    return "quat("..q.angle..", "..tostring(q.axis)..")";
end

_metatable_registry.quat.__tostring = format_quat

-- extra builtins

local
function log(fmt, ...)
    if type(fmt) == "string" and ... then
        amulet.log(string.format(fmt, ...), false, 2)
    else
        amulet.log(tostring(fmt), false, 2)
    end
end
rawset(_G, "log", log)

local
function log1(fmt, ...)
    if type(fmt) == "string" and ... then
        amulet.log(string.format(fmt, ...), true, 2)
    else
        amulet.log(tostring(fmt), true, 2)
    end
end
rawset(_G, "log1", log1)
