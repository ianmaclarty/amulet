local sections = {
    {id = "basic",      module = "_G",           title = "Basic Functions"},
    {id = "buffer",     module = "amulet",       title = "Buffers and Views"},
    {id = "window",     module = "amulet",       title = "Creating a Window"},
    {id = "graphics",   module = "amulet",       title = "Graphics"},
    {id = "text",       module = "amulet",       title = "Text"},
    {id = "audio",      module = "amulet",       title = "Audio"},
    {id = "input",      module = "amulet",       title = "User Input"},
    {id = "prefs",      module = "amulet",       title = "Preference Store"},
    {id = "config",     module = "amulet",       title = "Engine Configuration"},
    {id = "table",      module = "table",        title = "Tables"},
    {id = "math",       module = "math",         title = "Maths"},
    {id = "string",     module = "string",       title = "Strings"},
    {id = "coroutine",  module = "coroutine",    title = "Coroutines"},
    {id = "os",         module = "os",           title = "System Functions"},
    {id = "io",         module = "io",           title = "File I/O"},
}

local ignored_funcs = {
    "_G.module",
    "_G.loadstring",
    "_G.load",
    "_G.loadfile",
    "_G.dofile",
}

local modules = {}
for _, section in ipairs(sections) do
    if not table.search(modules, section.module) then
        table.insert(modules, section.module)
    end
end

local lua_html_manual = arg[1]
local am_func_list = require "functions"
local am_funcs = {}
for _, func in ipairs(am_func_list) do
    am_funcs[func.module.."."..func.name] = func
end

local
function read_lines(file)
    local f = io.open(file)
    local lines = {}
    local i = 1
    for line in f:lines() do
        lines[i] = line
        i = i + 1
    end
    f:close()
    return lines
end

local
function extract_lua_builtins(file)
    local lines = read_lines(file)
    local l = 1
    local n = #lines
    local funcs = {}
    while true do
        local line = lines[l]
        if not line then
            break
        end
        local name, sig = line:match('^<hr><h3><a name="pdf%-([a-zA-Z0-9%.]+)"><code>(.+)</code></a></h3>')
        if name then
            local module = "_G"
            if name:match("%.") then
                module, name = name:match("^(.*)%.(.*)$")
            end
            local qname = module.."."..name
            if table.search(ignored_funcs, qname) then
                goto continue
            end
            l = l + 1
            local description = ""
            while true do
                local line = lines[l]
                if line:match("^<h[1-3]>") or
                    line:match("<p>") and l < n and lines[l+1]:match("^<hr>")
                then
                    break
                end
                description = description..line.."\n"
                l = l + 1
            end
            local func = {
                name = name,
                module = module,
                signature = sig,
                description = description,
                section = module == "_G" and "basic" or module,
                is_lua_builtin = true,
            }
            funcs[qname] = func
        end
        ::continue::
        l = l + 1
    end
    return funcs
end

local
function check_amulet_funcs(lua_funcs, am_funcs)
    local funcs = {}
    for _, module in ipairs(modules) do
        for name, value in pairs(_G[module]) do
            local qname = module.."."..name
            if type(value) == "function" and not lua_funcs[qname] and name:sub(1, 1) ~= "_" and not table.search(ignored_funcs, qname) then
                local func = am_funcs[qname]
                if not func then
                    io.stderr:write("no doc entry for "..qname.."\n")
                elseif not func.signature then
                    io.stderr:write("no signature for "..qname.."\n")
                elseif not func.description then
                    io.stderr:write("no description for "..qname.."\n")
                elseif not func.section then
                    io.stderr:write("no section for "..qname.."\n")
                end
            end
        end
    end
end

local
function gen_func_html(func)
    local class = "func-entry"
    if func.is_lua_builtin then
        class = class.." lua-builtin"
    end
    local qname
    if func.module == "_G" then
        qname = func.name
    else
        qname = func.module.."."..func.name
    end
    io.stdout:write('<div class="'..class..'"><h3><a name="pdf-'..qname..'"><code>'..func.signature..'</code></a></h3>\n')
    io.stdout:write(func.description..'\n')
    io.stdout:write('</div>\n')
end

local
function gen_funcs_html(funcs)
    funcs = table.values(funcs)
    table.sort(funcs, function(f1, f2)
        return f1.name < f2.name
    end)
    for i, func in ipairs(funcs) do
        gen_func_html(func)
    end
end

local lua_funcs = extract_lua_builtins(lua_html_manual)

check_amulet_funcs(lua_funcs, am_funcs)

local all_funcs = table.shallow_copy(lua_funcs)
table.merge(all_funcs, am_funcs)
all_funcs = table.values(all_funcs)

for _, section in ipairs(sections) do
    local funcs = table.filter(all_funcs, function(f)
        return f.section == section.id
    end)
    io.stdout:write('<h2>'..section.title..'</h2>\n')
    gen_funcs_html(funcs)
end
