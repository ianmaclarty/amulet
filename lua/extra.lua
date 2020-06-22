-- extra table functions

local debug = _G.debug

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
            end
            if type(v) == "table" then
                v = deep_copy_2(v, seen)
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

function table.remove_all(t, val)
    for i = #t, 1, -1 do
        if t[i] == val then
            table.remove(t, i)
        end
    end
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

function table.count(t)
    local count = 0
    for k, v in pairs(t) do
        count = count + 1
    end
    return count
end

function table.shuffle(t, r)
    if type(t) ~= "table" then
        error("table expected, but got a "..type(t), 2)
    end

    local math_random = r or math.random

    for a = #t, 2, -1 do
        local b = math_random(a)
        t[a], t[b] = t[b], t[a]
    end
end

local
function table_tostring(field, t, indent, seen, depth)
    indent = indent or 0
    local tp = type(t)
    if tp == "table" then
        if seen[t] then
            error("cycle detected at field "..tostring(field), depth + 2)
        else
            seen[t] = true
        end
        local tab = "    "
        local prefix = string.rep(tab, indent)
        local str = "{\n"
        local keys = {}
        local array_test = 1
        for key, _ in pairs(t) do
            table.insert(keys, key)
            if key ~= array_test then
                array_test = nil
            else
                array_test = array_test + 1
            end
        end
        if array_test then
            for i = 1, array_test - 1 do
                str = str .. prefix .. tab .. table_tostring(i, t[i], indent + 1, seen, depth + 1) .. ",\n"
            end
        else
            table.sort(keys, function(k1, k2)
                local t1 = type(k1)
                local t2 = type(k2)
                if t1 == "string" and t2 == "string" then
                    return k1 < k2
                end
                if t1 == "number" and t2 == "number" then
                    return k1 < k2
                end
                if t1 == "string" and t2 == "number" then
                    return true
                end
                if t1 == "number" and t2 == "string" then
                    return false
                end
                return tostring(k1) < tostring(k2)
            end)
            for _, key in ipairs(keys) do
                local value = t[key]
                local keystr
                if type(key) == "string" and key:match"^[A-Za-z_][A-Za-z0-9_]*$" then
                    keystr = key
                else
                    keystr = "["..table_tostring("<key>", key, 0, seen, depth + 1).."]"
                end
                local valstr = table_tostring(key, value, indent + 1, seen, depth + 1)
                str = str .. prefix .. tab .. keystr .. " = " .. valstr .. ",\n"
            end
        end
        str = str .. prefix .. "}"
        seen[t] = nil
        return str
    elseif tp == "string" then
        return '"' .. t:gsub("\"", "\\\""):gsub("%\n", "\\n") .. '"'
    else
        return tostring(t)
    end
end

table.tostring = function(t, indent)
    return table_tostring("<root>", t, indent, {}, 1)
end

-- extra math functions

function math.randvec2()
    return vec2(math.random(), math.random())
end

function math.randvec3()
    return vec3(math.random(), math.random(), math.random())
end

function math.randvec4()
    return vec4(math.random(), math.random(), math.random(), math.random())
end

function math.sign(n)
    return n > 0 and 1 or n < 0 and -1 or 0
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
function format_num(n)
    local str = string.format("%.16g", n)
    if str == "-0" then
        return "0"
    end
    return str
end

local
function format_vec(v)
    local n = #v
    local str = "vec"..n.."("
    for i = 1, n do
        str = str..format_num(v[i])
        if i == n then
            str = str..")"
        else
            str = str..", "
        end
    end
    return str
end

local
function default_concat(a,b)
    return tostring(a)..tostring(b)
end

_metatable_registry.vec2.__tostring = format_vec
_metatable_registry.vec3.__tostring = format_vec
_metatable_registry.vec4.__tostring = format_vec

_metatable_registry.vec2.__concat = default_concat
_metatable_registry.vec3.__concat = default_concat
_metatable_registry.vec4.__concat = default_concat

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
            str = str..string.format(fmt, format_num(m[col][row]))
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

_metatable_registry.mat2.__concat = default_concat
_metatable_registry.mat3.__concat = default_concat
_metatable_registry.mat4.__concat = default_concat

local
function format_quat(q)
    return "quat("..q.angle..", "..tostring(q.axis)..")";
end

_metatable_registry.quat.__tostring = format_quat

_metatable_registry.quat.__concat = default_concat

-- extra builtins

local log_overlay_node
local log_overlay_lines
local max_log_lines = 20
local first_log = true
local
function log(fmt, ...)
    local msg
    if type(fmt) == "string" and ... then
        local args = {}
        for a = 1, select("#", ...) do
            local arg = select(a, ...)
            if type(arg) == "userdata" then
                arg = tostring(arg)
            end
            table.insert(args, arg)
        end
        msg = string.format(fmt, unpack(args))
    else
        msg = tostring(fmt)
    end

    local info = debug.getinfo(2, "Sl")
    if info then
        msg = info.short_src..":"..info.currentline..": "..msg
    end
    am.log(msg, false, 2)

    -- write to log file if on desktop system
    local plat = am.platform
    if plat == "windows" or plat == "linux" or plat == "osx" then
        local f = io.open(am.app_data_dir.."log.txt", first_log and "w" or "a")
        if f then
            f:write(msg)
            f:write("\n")
            f:close()
        end
    end
    
    if am._main_window then
        local win = am._main_window
        if not log_overlay_node then
            log_overlay_node = am.depth_test("always", false)
                ^ am.bind{P = math.ortho(0, win.pixel_width, 0, win.pixel_height)}
                ^ am.translate(0, win.pixel_height)
                ^ {
                    am.rect(0, 0, 0, 0, vec4(0, 0, 0, 0.5)),
                    am.text("", vec4(0, 1, 0, 1), "left", "top")
                }
            log_overlay_node:action(function(node)
                if win:resized() then
                    node.P = math.ortho(0, win.pixel_width, 0, win.pixel_height)
                    node"translate".y = win.pixel_height
                end
            end)
            if not win._overlay then
                win._overlay = am.group()
            end
            win._overlay:append(log_overlay_node)
            log_overlay_lines = {}
        end
        table.insert(log_overlay_lines, msg)
        while #log_overlay_lines > max_log_lines do
            table.remove(log_overlay_lines, 1)
        end
        local str = ""
        for i = 1, #log_overlay_lines do
            str = str..log_overlay_lines[i]
            if i < #log_overlay_lines then
                str = str.."\n"
            end
        end
        local text_node = log_overlay_node"text"
        text_node.text = str
        local rect = log_overlay_node"rect"
        rect.x2 = text_node.width
        rect.y1 = -text_node.height
    end

    first_log = false
end
rawset(_G, "log", log)

local
function log1(fmt, ...)
    if type(fmt) == "string" and ... then
        am.log(string.format(fmt, ...), true, 2)
    else
        am.log(tostring(fmt), true, 2)
    end
end
rawset(_G, "log1", log1)

local
function noglobals()
    setmetatable(_G, {
        __index = function(t, k)
            error("attempt to reference missing global "..tostring(k), 2)
        end,
        __newindex = function(t, k, v)
            error("attempt to set global "..tostring(k), 2)
        end,
    })
end

rawset(_G, "noglobals", noglobals)
