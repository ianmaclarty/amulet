#!/usr/bin/env amulet

local func_defs = {
    -- basic functions
    { 
        name = "add",
        kind = "component_wise",
        comps = {1, 2, 3, 4, 9, 16},
        variants = {
            {
                cname = "ADD_F32",
                ret_type = "f32",
                args = {
                    {name = "x", type = "f32"},
                    {name = "y", type = "f32"},
                },
            },
        }
    },
    { 
        name = "sub",
        kind = "component_wise",
        comps = {1, 2, 3, 4, 9, 16},
        variants = {
            {
                cname = "SUB_F32",
                ret_type = "f32",
                args = {
                    {name = "x", type = "f32"},
                    {name = "y", type = "f32"},
                },
            },
        }
    },
    { 
        name = "mul",
        kind = "component_wise",
        comps = {1, 2, 3, 4},
        variants = {
            {
                cname = "MUL_F32",
                ret_type = "f32",
                args = {
                    {name = "x", type = "f32"},
                    {name = "y", type = "f32"},
                },
            },
        }
    },
    { 
        name = "div",
        kind = "component_wise",
        comps = {1, 2, 3, 4},
        variants = {
            {
                cname = "DIV_F32",
                ret_type = "f32",
                args = {
                    {name = "x", type = "f32"},
                    {name = "y", type = "f32"},
                },
            },
        }
    },
    { 
        name = "mod",
        kind = "component_wise",
        comps = {1, 2, 3, 4},
        variants = {
            {
                cname = "fmodf",
                ret_type = "f32",
                args = {
                    {name = "x", type = "f32"},
                    {name = "y", type = "f32"},
                },
            },
        }
    },
    { 
        name = "pow",
        kind = "component_wise",
        comps = {1, 2, 3, 4},
        variants = {
            {
                cname = "powf",
                ret_type = "f32",
                args = {
                    {name = "x", type = "f32"},
                    {name = "y", type = "f32"},
                },
            },
        }
    },
    { 
        name = "unm",
        kind = "component_wise",
        comps = {1, 2, 3, 4},
        variants = {
            {
                cname = "UNM_F32",
                ret_type = "f32",
                args = {
                    {name = "x", type = "f32"},
                },
            },
        }
    },

    -- component-wise math functions
    { 
        name = "sin",
        kind = "component_wise",
        comps = {1},
        variants = {
            {
                cname = "sinf",
                ret_type = "f32",
                args = {
                    {name = "angle", type = "f32"}
                },
            },
        }
    },

    -- element-wise math functions
    {
        name = "vec2",
        kind = "element_wise",
        variants = {
            {
                cname = "glm::vec2",
                ret_type = "f32",
                ret_comps = 2,
                args = {
                    {name = "x", type = "f32", comps = 1},
                    {name = "y", type = "f32", comps = 1},
                },
            },
            {
                cname = "glm::vec2",
                ret_type = "f32",
                ret_comps = 2,
                args = {
                    {name = "xy", type = "f32", comps = 2},
                },
            },
        }
    },
    {
        name = "vec3",
        kind = "element_wise",
        variants = {
            {
                cname = "glm::vec3",
                ret_type = "f32",
                ret_comps = 3,
                args = {
                    {name = "x", type = "f32", comps = 1},
                    {name = "y", type = "f32", comps = 1},
                    {name = "z", type = "f32", comps = 1},
                },
            },
            {
                cname = "glm::vec3",
                ret_type = "f32",
                ret_comps = 3,
                args = {
                    {name = "xy", type = "f32", comps = 2},
                    {name = "z", type = "f32", comps = 1},
                },
            },
            {
                cname = "glm::vec3",
                ret_type = "f32",
                ret_comps = 3,
                args = {
                    {name = "x", type = "f32", comps = 1},
                    {name = "yz", type = "f32", comps = 2},
                },
            },
            {
                cname = "glm::vec3",
                ret_type = "f32",
                ret_comps = 3,
                args = {
                    {name = "xyz", type = "f32", comps = 3},
                },
            },
        }
    },
    {
        name = "vec4",
        kind = "element_wise",
        variants = {
            {
                cname = "glm::vec4",
                ret_type = "f32",
                ret_comps = 4,
                args = {
                    {name = "x", type = "f32", comps = 1},
                    {name = "y", type = "f32", comps = 1},
                    {name = "z", type = "f32", comps = 1},
                    {name = "w", type = "f32", comps = 1},
                },
            },
            {
                cname = "glm::vec4",
                ret_type = "f32",
                ret_comps = 4,
                args = {
                    {name = "xyz", type = "f32", comps = 3},
                    {name = "w", type = "f32", comps = 1},
                },
            },
            {
                cname = "glm::vec4",
                ret_type = "f32",
                ret_comps = 4,
                args = {
                    {name = "x", type = "f32", comps = 1},
                    {name = "yzw", type = "f32", comps = 3},
                },
            },
            {
                cname = "glm::vec4",
                ret_type = "f32",
                ret_comps = 4,
                args = {
                    {name = "xy", type = "f32", comps = 2},
                    {name = "zw", type = "f32", comps = 2},
                },
            },
            {
                cname = "glm::vec4",
                ret_type = "f32",
                ret_comps = 4,
                args = {
                    {name = "x", type = "f32", comps = 1},
                    {name = "y", type = "f32", comps = 1},
                    {name = "zw", type = "f32", comps = 2},
                },
            },
            {
                cname = "glm::vec4",
                ret_type = "f32",
                ret_comps = 4,
                args = {
                    {name = "x", type = "f32", comps = 1},
                    {name = "yz", type = "f32", comps = 2},
                    {name = "w", type = "f32", comps = 1},
                },
            },
            {
                cname = "glm::vec4",
                ret_type = "f32",
                ret_comps = 4,
                args = {
                    {name = "xy", type = "f32", comps = 2},
                    {name = "z", type = "f32", comps = 1},
                    {name = "w", type = "f32", comps = 1},
                },
            },
            {
                cname = "glm::vec4",
                ret_type = "f32",
                ret_comps = 4,
                args = {
                    {name = "xyzw", type = "f32", comps = 4},
                },
            },
        }
    },
    {
        name = "dot",
        kind = "element_wise",
        variants = {
            {
                cname = "glm::dot",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 2},
                    {name = "y", type = "f32", comps = 2},
                },
            },
            {
                cname = "glm::dot",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 3},
                    {name = "y", type = "f32", comps = 3},
                },
            },
            {
                cname = "glm::dot",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 4},
                    {name = "y", type = "f32", comps = 4},
                },
            },
        }
    },
    {
        name = "perlin",
        kind = "element_wise",
        variants = {
            {
                cname = "PERLIN1_F32",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 1},
                },
            },
            {
                cname = "glm::perlin",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 2},
                },
            },
            {
                cname = "glm::perlin",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 3},
                },
            },
            {
                cname = "glm::perlin",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 4},
                },
            },
            {
                cname = "PERLIN2_F32",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 1},
                    {name = "y", type = "f32", comps = 1},
                },
            },
            {
                cname = "glm::perlin",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 2},
                    {name = "y", type = "f32", comps = 2},
                },
            },
            {
                cname = "glm::perlin",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 3},
                    {name = "y", type = "f32", comps = 3},
                },
            },
            {
                cname = "glm::perlin",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 4},
                    {name = "y", type = "f32", comps = 4},
                },
            },
        }
    },
    {
        name = "simplex",
        kind = "element_wise",
        variants = {
            {
                cname = "SIMPLEX1_F32",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 1},
                },
            },
            {
                cname = "glm::simplex",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 2},
                },
            },
            {
                cname = "glm::simplex",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 3},
                },
            },
            {
                cname = "glm::simplex",
                ret_type = "f32",
                ret_comps = 1,
                args = {
                    {name = "x", type = "f32", comps = 4},
                },
            },
        }
    },
}

local view_type_info = {
    f32 = {enumval = "AM_VIEW_TYPE_F32", ctype = "float", vec_ctypes = {"float", "glm::vec2", "glm::vec3", "glm::vec4"}},
}

local
function ind(f, n, str)
    if not str then
        str = n
        n = f
    end
    local suf = string.rep("    ", n)
    local pad = str:match("^(%s*)")
    local r = ""
    for line in str:gmatch("([^\n\r]*)\r*\n") do
        r = r..suf..(line:sub(#pad + 1)).."\n"
    end
    if f then
        f:write(r)
    end
    return r
end

local
function keys(t)
    local ks = {}
    local i = 1
    for k, _ in pairs(t) do
        ks[i] = k
        i = i + 1
    end
    return ks
end

local
function get_max_args(func)
    local m = 0
    for _, variant in ipairs(func.variants) do
        if #variant.args > m then
            m = #variant.args
        end
    end
    return m
end

local
function get_arg_types(func, except)
    local types = {}
    for _, variant in ipairs(func.variants) do
        for _, arg in ipairs(variant.args) do
            if arg.type ~= except then
                types[arg.type] = true
            end
        end
    end
    return keys(types)
end

local gen_funcs
local gen_func
local gen_component_wise_func_body
local gen_component_wise_func_variant
local gen_component_wise_case
local gen_element_wise_func_body
local gen_element_wise_func_variant
local gen_element_wise_case

function gen_funcs(f)
    for _, func in ipairs(func_defs) do
        gen_func(f, func)
    end
end

function gen_func(f, func)
    local max_args = get_max_args(func)
    local sig_error = "invalid argument types for function mathv."..func.name..".\\nsupported signatures are:\\n"
    for _, variant in ipairs(func.variants) do
        sig_error = sig_error.."  mathv."..func.name.."("
        for i, arg in ipairs(variant.args) do
            sig_error = sig_error..arg.name..":"..arg.type
            if i < #variant.args then
                sig_error = sig_error..", "
            end
        end
        sig_error = sig_error..")\\n"
    end
    local return_sig_error = "return luaL_error(L, \""..sig_error.."\");\n"
    local cfunc_name = "am_mathv_"..func.name
    local func_decl = "int "..cfunc_name.."(lua_State *L) {\n"
    local prelude = [[
        int nargs = lua_gettop(L);
        if (nargs > ]]..max_args..[[) return luaL_error(L, "too many arguments for mathv.]]..func.name..[[");
        double arg_singleton_vals[]]..max_args..[[][16];
        int arg_count[]]..max_args..[[];
        uint8_t *arg_data[]]..max_args..[[];
        int arg_stride[]]..max_args..[[];
        int arg_components[]]..max_args..[[];
        int arg_type[]]..max_args..[[];
        am_buffer_view_type arg_view_type[]]..max_args..[[];
        for (int i = 0; i < nargs; i++) {
            if (!read_arg(L, i+1, &arg_type[i], &arg_view_type[i], &arg_data[i], &arg_stride[i], &arg_count[i], &arg_components[i], &arg_singleton_vals[i][0])) {
                ]]..return_sig_error..[[
            }
        }
    ]]
    ind(f, 0, func_decl)
    ind(f, 1, prelude)
    if func.kind == "component_wise" then
        gen_component_wise_func_body(f, func)
    elseif func.kind == "element_wise" then
        gen_element_wise_func_body(f, func)
    else
        error("unknown function kind: "..func.kind)
    end

    ind(f, 1, return_sig_error)
    ind(f, 0, "}\n\n")
end

function gen_component_wise_func_body(f, func)
    local check_zero_args = [[
        // code below depends on there being at least one arg
        if (nargs == 0) return luaL_error(L, "no arguments given for mathv.]]..func.name..[[");
    ]]

    local compute_count = [[
        // compute count
        int count = -1;
        for (int i = 0; i < nargs; i++) {
            if (arg_type[i] == MT_am_buffer_view) {
                if (count != -1 && arg_count[i] != count) {
                    return luaL_error(L, "in call to mathv.]]..func.name..[[ argument %d has size %d, but previous arguments have size %d", (i+1), arg_count[i], count);
                } else if (count == -1) {
                    count = arg_count[i];
                }
            }
        }
        bool no_view_args = false;
        if (count == -1) {
            no_view_args = true;
            count = 1;
        }
    ]]

    local compute_output_components = [[
        int output_components = arg_components[0];
        for (int i = 1; i < nargs; i++) {
            if (output_components != arg_components[i]) {
                if (output_components == 1) {
                    output_components = arg_components[i];
                } else if (arg_components[i] != 1) {
                    return luaL_error(L, "in call to ]]..func.name..[[ argument %d has %d components, but previous arguments have %d components",
                        i+1, arg_components[i], output_components);
                }
            }
        }
    ]]

    ind(f, 1, check_zero_args)
    ind(f, 1, compute_count)
    ind(f, 1, compute_output_components)

    for _, variant in ipairs(func.variants) do
        gen_component_wise_func_variant(f, func, variant)
    end
end

function gen_component_wise_func_variant(f, func, variant)
    local args = variant.args
    local nargs = #args
    local sig_test = "nargs == "..nargs.." "
    for ii, arg in ipairs(args) do
        local i = ii-1
        local enumval = view_type_info[arg.type].enumval
        sig_test = sig_test.." && ((arg_type["..i.."] == MT_am_buffer_view && arg_view_type["..i.."] == "..enumval..") || "..
            "arg_type["..i.."] != MT_am_buffer_view)"
    end

    local setup_non_view_arg_data = "// setup non-view args\n"
    for ii, arg in ipairs(args) do
        local i = ii-1
        local setup = [[
if (arg_type[]]..i..[[] != MT_am_buffer_view) {
    double *f64s = &arg_singleton_vals[]]..i..[[][0];
        ]]
        if arg.type ~= "f64" then
            -- convert doubles to required type, overwriting the doubles
            -- (a little messy, but should be fine, because the required type will be smaller or
            -- of equal size and we don't need the doubles anymore)
            local ctype = view_type_info[arg.type].ctype
            setup = setup..[[
    ]]..ctype..[[ *conv = (]]..ctype..[[*)f64s;
    for (int i = 0; i < arg_components[]]..i..[[]; i++) {
        conv[i] = (]]..ctype..[[)f64s[i];
    }
            ]]
        end
        setup = setup..[[
    arg_data[]]..i..[[] = (uint8_t*)f64s;
}
        ]]

        setup_non_view_arg_data = setup_non_view_arg_data..setup
    end

    local ret_ctype = view_type_info[variant.ret_type].ctype
    local ret_view_type = view_type_info[variant.ret_type].enumval
    local component_size = "sizeof("..ret_ctype..")"
    -- TODO handle no_view_args
    local create_return_value = [[
        uint8_t *output_ptr;
        int out_stride = output_components * ]]..component_size..[[;

        am_buffer *output_buffer = am_push_new_buffer_and_init(L, count * output_components * ]]..component_size..[[);
        output_ptr = output_buffer->data;
        am_buffer_view *output_view = am_new_buffer_view(L, ]]..ret_view_type..[[, output_components);
        output_view->buffer = output_buffer;
        output_view->buffer_ref = output_view->ref(L, -2);
        output_view->offset = 0;
        output_view->stride = out_stride;
        output_view->size = count;
        output_view->last_max_elem_version = 0;
        output_view->max_elem = 0;

        lua_remove(L, -2); // remove output_buffer
    ]]

    local default_case = [[
        default:
            return luaL_error(L, "internal error: unexpected number of output components: %d", output_components);
    ]]

    ind(f, 1, "if ("..sig_test..") {\n")
    ind(f, 2, setup_non_view_arg_data)
    ind(f, 2, create_return_value)
    ind(f, 2, "switch(output_components) {\n");
    for _, c in ipairs(func.comps) do
        gen_component_wise_case(f, func, variant, c)
    end
    ind(f, 3, default_case);
    ind(f, 2, "      }\n");
    ind(f, 1, "  }\n")
end

function gen_component_wise_case(f, func, variant, components)
    local case_begin = [[
        case ]]..components..[[: {
    ]]
    local setup_pointers = ""
    for a, arg in ipairs(variant.args) do
        local comp_size = "sizeof("..view_type_info[arg.type].ctype..")"
        setup_pointers = setup_pointers..[[
int in_stride_]]..a..[[ = arg_stride[]]..(a-1)..[[];
]]
        for c = 1, components do
            local ptr_name = "in_ptr_"..a.."_"..c
            setup_pointers = setup_pointers..[[
uint8_t *]]..ptr_name..[[;
if (arg_components[]]..(a-1)..[[] > 1) {
    ]]..ptr_name..[[ = &arg_data[]]..(a-1)..[[][0] + ]]..(c-1)..[[ * ]]..comp_size..[[;
} else {
    ]]..ptr_name..[[ = &arg_data[]]..(a-1)..[[][0];
}
]]
        end
    end
    local ret_ctype = view_type_info[variant.ret_type].ctype
    local ret_compsize = "sizeof("..ret_ctype..")"
    for c = 1, components do
        setup_pointers = setup_pointers..[[
uint8_t *out_ptr_]]..c..[[ = output_ptr + ]]..(c-1)..[[ * ]]..ret_compsize..[[;
]]
    end
    local iterate = [[
for (int i = 0; i < count; ++i) {
]]
    for c = 1, components do
        iterate = iterate.."    *(("..ret_ctype.."*)out_ptr_"..c..") = "..variant.cname.."("
        for a, arg in ipairs(variant.args) do
            local ctype = view_type_info[arg.type].ctype
            iterate = iterate.."*(("..ctype.."*)in_ptr_"..a.."_"..c..")"
            if a < #variant.args then
                iterate = iterate..", "
            end
        end
        iterate = iterate..");\n"
        for a, arg in ipairs(variant.args) do
            iterate = iterate.."    in_ptr_"..a.."_"..c.." += in_stride_"..a..";\n"
        end
        iterate = iterate.."    out_ptr_"..c.." += out_stride;\n";
    end
    iterate = iterate.."}\n"
    local case_end = "return 1;\n"
    ind(f, 3, case_begin)
    ind(f, 4, setup_pointers)
    ind(f, 4, iterate)
    ind(f, 4, case_end)
    ind(f, 3, "}\n")
end

function gen_element_wise_func_body(f, func)
    local check_zero_args = [[
        // code below depends on there being at least one arg
        if (nargs == 0) return luaL_error(L, "no arguments given for mathv.]]..func.name..[[");
    ]]

    local compute_count = [[
        // compute count
        int count = -1;
        for (int i = 0; i < nargs; i++) {
            if (arg_type[i] == MT_am_buffer_view) {
                if (count != -1 && arg_count[i] != count) {
                    return luaL_error(L, "in call to mathv.]]..func.name..[[ argument %d has size %d, but previous arguments have size %d", (i+1), arg_count[i], count);
                } else if (count == -1) {
                    count = arg_count[i];
                }
            }
        }
        bool no_view_args = false;
        if (count == -1) {
            no_view_args = true;
            count = 1;
        }
    ]]

    ind(f, 1, check_zero_args)
    ind(f, 1, compute_count)

    for _, variant in ipairs(func.variants) do
        gen_element_wise_func_variant(f, func, variant)
    end
end

function gen_element_wise_func_variant(f, func, variant)
    local args = variant.args
    local nargs = #args
    local sig_test = "nargs == "..nargs.." "
    for ii, arg in ipairs(args) do
        local i = ii-1
        local enumval = view_type_info[arg.type].enumval
        sig_test = sig_test.." && arg_components["..i.."] == "..arg.comps..
            " && ((arg_type["..i.."] == MT_am_buffer_view && arg_view_type["..i.."] == "..enumval..") || "..
            "arg_type["..i.."] != MT_am_buffer_view)"
    end

    local setup_non_view_arg_data = "// setup non-view args\n"
    for ii, arg in ipairs(args) do
        local i = ii-1
        local setup = [[
if (arg_type[]]..i..[[] != MT_am_buffer_view) {
    double *f64s = &arg_singleton_vals[]]..i..[[][0];
]]
        if arg.type ~= "f64" then
            -- convert doubles to required type, overwriting the doubles
            -- (a little messy, but should be fine, because the required type will be smaller or
            -- of equal size and we don't need the doubles anymore)
            local ctype = view_type_info[arg.type].ctype
            setup = setup..[[
    ]]..ctype..[[ *conv = (]]..ctype..[[*)f64s;
    for (int i = 0; i < arg_components[]]..i..[[]; i++) {
        conv[i] = (]]..ctype..[[)f64s[i];
    }
]]
      end
        setup = setup..[[
    arg_data[]]..i..[[] = (uint8_t*)f64s;
}
]]

        setup_non_view_arg_data = setup_non_view_arg_data..setup
    end

    local ret_components = variant.ret_comps
    local ret_ctype = view_type_info[variant.ret_type].ctype
    local ret_view_type = view_type_info[variant.ret_type].enumval
    local ret_compsize = "sizeof("..ret_ctype..")"
    -- TODO handle no_view_args
    local create_return_value = [[
        uint8_t *output_ptr;
        int out_stride = ]]..ret_compsize..[[ * ]]..ret_components..[[;

        am_buffer *output_buffer = am_push_new_buffer_and_init(L, count * out_stride);
        output_ptr = output_buffer->data;
        am_buffer_view *output_view = am_new_buffer_view(L, ]]..ret_view_type..[[, ]]..ret_components..[[);
        output_view->buffer = output_buffer;
        output_view->buffer_ref = output_view->ref(L, -2);
        output_view->offset = 0;
        output_view->stride = out_stride;
        output_view->size = count;
        output_view->last_max_elem_version = 0;
        output_view->max_elem = 0;

        lua_remove(L, -2); // remove output_buffer
    ]]

    local setup_pointers = ""
    for a, arg in ipairs(variant.args) do
        local comp_size = "sizeof("..view_type_info[arg.type].ctype..")"
        setup_pointers = setup_pointers..[[
int in_stride_]]..a..[[ = arg_stride[]]..(a-1)..[[];
uint8_t *in_ptr_]]..a..[[ = &arg_data[]]..(a-1)..[[][0];
]]
    end

    out_ctype_vec = view_type_info[variant.ret_type].vec_ctypes[variant.ret_comps]
    local iterate = [[
for (int i = 0; i < count; ++i) {
    *((]]..out_ctype_vec..[[*)output_ptr) = ]]..variant.cname..[[(]]
    for a, arg in ipairs(variant.args) do
        local ctype = view_type_info[arg.type].vec_ctypes[arg.comps]
        iterate = iterate.."*(("..ctype.."*)in_ptr_"..a..")"
        if a < #variant.args then
            iterate = iterate..", "
        end
    end
    iterate = iterate..");\n"
    for a, arg in ipairs(variant.args) do
        iterate = iterate.."    in_ptr_"..a.." += in_stride_"..a..";\n"
    end
    iterate = iterate.."    output_ptr += out_stride;\n";
    iterate = iterate.."}\n"

    ind(f, 1, "if ("..sig_test..") {\n")
    ind(f, 2, setup_non_view_arg_data)
    ind(f, 2, create_return_value)
    ind(f, 2, setup_pointers)
    ind(f, 2, iterate)
    ind(f, 2, "return 1;\n")
    ind(f, 1, "}\n")
end

local
function gen_open_module_func(f)
    f:write([[
void am_open_mathv_module(lua_State *L) {
    luaL_Reg vfuncs[] = {
        {"range", am_mathv_range},
        {"random", am_mathv_random},
        {"cart", am_mathv_cart},
]])
    for _, func in ipairs(func_defs) do
        f:write("    {\""..func.name.."\", am_mathv_"..func.name.."},\n")
    end
    f:write([[
        {NULL, NULL}
    };
    am_open_module(L, "mathv", vfuncs);
}
]])

end

function gen_func_headers(f)
    for _, func in ipairs(func_defs) do
        f:write("int am_mathv_"..func.name.."(lua_State *L);\n")
    end
end

local
function gen_cpp_file()
    local f = io.open("src/am_mathv.cpp", "w")
    ind(f, 0, [[
// This file is generated by tools/gen_mathv.lua
#include "amulet.h"
#include "am_mathv_helper.inc"
    ]])
    gen_funcs(f)
    gen_open_module_func(f)
    f:close()
end

local
function gen_h_file()
    local f = io.open("src/am_mathv.h", "w")
    ind(f, 0, [[
// This file is generated by tools/gen_mathv.lua
void am_open_mathv_module(lua_State *L);
    ]])
    gen_func_headers(f)
    f:close()
end

--gen_h_file()
gen_cpp_file()
