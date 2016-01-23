local
function qname(func)
    return func.module.."."..func.name
end

local lua_html_manual = arg[1]

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
        local name, params = line:match('^<%/p><hr><h3><a name="pdf%-([a-zA-Z0-9%.]+)"><code>[a-zA-Z0-9_]+%s*%(%s*(.+)%s*%)%s*</code></a></h3>')
        if name then
            local module = "_G"
            if name:match("%.") then
                module, name = name:match("^(.*)%.(.*)$")
            end
            local qname = module.."."..name
            l = l + 1
            local description = ""
            while true do
                local line = lines[l]
                if line:match("^<%/p><h[1-3]>") or
                    line:match("<p>") and l < n and lines[l+1]:match("^<%/p><hr>")
                then
                    break
                end
                description = description..line.."\n"
                l = l + 1
            end
            local func = {
                name = name,
                module = module,
                params = params,
                description = description,
                section = module == "_G" and "basic" or module,
                is_lua_builtin = true,
            }
            funcs[qname] = func
        end
        l = l + 1
    end
    return funcs
end

local
function gen_func_md(func)
    local class = ".func-def"
    if func.is_lua_builtin then
        class = class.." .lua-builtin"
    end
    local qname
    if func.module == "_G" then
        qname = func.name
    else
        qname = func.module.."."..func.name
    end
    io.stdout:write(string.format("### %s(%s) {#%s %s}\n\n",
        qname, func.params, qname, class))
    local descr = func.description:gsub("<%/?code>", "`")
        :gsub("<%/?p>", "\n\n")
    io.stdout:write(descr)
    io.stdout:write('\n\n')
end

local
function gen_funcs_md(funcs)
    funcs = table.values(funcs)
    table.sort(funcs, function(f1, f2)
        return qname(f1) < qname(f2)
    end)
    for i, func in ipairs(funcs) do
        gen_func_md(func)
    end
end

local lua_funcs = extract_lua_builtins(lua_html_manual)
--print(table.tostring(lua_funcs))

gen_funcs_md(lua_funcs)
