local am = amulet

local
function make_buffer(num_verts)
    local buffer = am.buffer(num_verts * 20)
    local verts = buffer:view("vec3", 0, 20)
    local uvs = buffer:view("vec2", 12, 20)
    return buffer, verts, uvs
end

local
function make_indices(num_verts)
    local num_tris = num_verts / 4
    local num_elems = num_tris * 6
    local arr_type
    local stride
    if num_elems < 2^16 then
        arr_type = "ushort_elem"
        stride = 2
    else
        arr_type = "uint_elem"
        stride = 4
    end
    local buffer = am.buffer(num_elems * stride)
    local view = buffer:view(arr_type, 0, stride)
    local indices = {}
    local i = 1
    local k = 0
    for j = 0, num_tris - 1 do
        indices[i]     = 1 + k
        indices[i + 1] = 2 + k
        indices[i + 2] = 3 + k
        indices[i + 3] = 1 + k
        indices[i + 4] = 3 + k
        indices[i + 5] = 4 + k
        i = i + 6
        k = k + 4
    end
    view:set(indices)
    return view
end

local newline = string.byte("\n")
local space = string.byte(" ")
local asterix = string.byte("*")
local dash = string.byte("-")
local underscore = string.byte("_")

local
function set_text_verts(font, str, verts_view, uvs_view)
    local x = 0
    local y = 0
    local verts = {}
    local uvs = {}
    local v = 1
    local u = 1
    local chars = font.chars
    for p, c in utf8.codes(str) do
        local char_data, x1, y1, x2, y2, advance
        if c == newline then
            y = y - font.line_height
            x = 0
            x1, y1, x2, y2 = 0, 0, 0, 0
            char_data = chars[space]
            advance = 0
        else
            char_data = chars[c] or chars[0] or chars[space]
            x1, y1, x2, y2 = char_data.x1, char_data.y1, char_data.x2, char_data.y2
            advance = char_data.advance
        end
        verts[v]     = x + x1
        verts[v + 1] = y + y2
        verts[v + 2] = 0
        verts[v + 3] = x + x1
        verts[v + 4] = y + y1
        verts[v + 5] = 0
        verts[v + 6] = x + x2
        verts[v + 7] = y + y1
        verts[v + 8] = 0
        verts[v + 9] = x + x2
        verts[v + 10] = y + y2
        verts[v + 11] = 0
        v = v + 12
        local s1, t1, s2, t2 = char_data.s1, char_data.t1, char_data.s2, char_data.t2
        uvs[u]     = s1
        uvs[u + 1] = t2
        uvs[u + 2] = s1
        uvs[u + 3] = t1
        uvs[u + 4] = s2
        uvs[u + 5] = t1
        uvs[u + 6] = s2
        uvs[u + 7] = t2
        u = u + 8
        x = x + advance
    end
    verts_view:set(verts)
    uvs_view:set(uvs)
end

local
function set_sprite_verts(img, verts_view, uvs_view, halign, valign)
    local x1, y1, x2, y2 = img.x1, img.y1, img.x2, img.y2
    local s1, t1, s2, t2 = img.s1, img.t1, img.s2, img.t2
    local w = img.w
    local h = img.h
    local x_os, y_os = 0, 0
    if halign == "center" then
        x_os = -w/2
    elseif halign == "right" then
        x_os = -w
    end
    if valign == "center" then
        y_os = -h/2
    elseif valign == "top" then
        y_os = -h
    end
    verts_view:set{x1 + x_os, y2 + y_os, 0, x1 + x_os, y1 + y_os, 0, x2 + x_os, y1 + y_os, 0, x2 + x_os, y2 + y_os, 0}
    uvs_view:set{s1, t2, s1, t1, s2, t1, s2, t2}
end

local
function set_text(node, str)
    local len = utf8.len(str)
    local verts, uvs
    if len > node.len then
        local num_verts = len * 4
        node.buffer:release_vbo()
        node.buffer, verts, uvs = make_buffer(num_verts)
        node.verts = verts
        node.uvs = uvs
    else
        verts = node.verts
        uvs = node.uvs
    end
    set_text_verts(node.font, str, verts, uvs)
    node.len = len
    node.str = str
end

function am.text(font, str)
    if not font.is_font then
        error("expecting a font in position 1", 2)
    end
    str = type(str) == "string" and str or tostring(str)
    local len = utf8.len(str)
    local num_verts = len * 4
    local buffer, verts, uvs = make_buffer(num_verts)
    local indices = make_indices(num_verts)
    set_text_verts(font, str, verts, uvs)
    return am.draw_elements(indices)
        :bind_array("pos", verts)
        :bind_array("uv", uvs)
        :bind_sampler2d("tex", font.texture)
        :bind_program(am.shaders.texture)
        :blend("normal")
        :extend{
            get_text = function(node)
                return str
            end,
            set_text = function(node, str1)
                str1 = type(str1) == "string" and str1 or tostring(str1)
                local len1 = utf8.len(str1)
                if len1 > len then
                    local num_verts = len1 * 4 
                    buffer, verts, uvs = make_buffer(num_verts)
                    indices = make_indices(num_verts)
                    node.pos = verts
                    node.uv = uvs
                    node.elements = indices
                    len = len1
                end
                str = str1
                set_text_verts(font, str, verts, uvs)
                node.count = utf8.len(str) * 6
            end,
        }
end

function am.sprite(image, halign, valign)
    halign = halign or "center"
    valign = valign or "center"
    local num_verts = 4
    local buffer, verts, uvs = make_buffer(num_verts)
    local indices = make_indices(num_verts)
    set_sprite_verts(image, verts, uvs, halign, valign)
    return am.draw_elements(indices)
        :bind{
            pos = verts,
            uv = uvs,
            tex = image.texture,
        }
        :bind_program(am.shaders.texture)
        :blend("normal")
        :extend{
            get_sprite = function(node)
                return image
            end,
            set_sprite = function(node, img)
                image = img
                set_sprite_verts(image, verts, uvs, halign, valign)
            end,
        }
end

function am._init_fonts(data, imgfile)
    local fonts = {}
    for _, entry in ipairs(data) do
        local name = entry.filename:match("([^/%.:\\]+)%.([^/%.:\\]+)$")
        if entry.is_font then
            if not name then
                error("invalid font filename: "..entry.filename, 3)
            end
            entry.line_height = entry.size
            name = name..entry.size
        else
            if not name then
                error("invalid image filename: "..entry.filename, 3)
            end
        end
        data[name] = entry
    end
    local texture
    local
    function ensure_texture_loaded()
        if not texture then
            local img = am.load_image(imgfile);
            texture = am.texture2d{buffer = img.buffer, width = img.width, height = img.height,
                minfilter = "linear", magfilter = "linear"}
        end
    end
    setmetatable(fonts, {__index = function(fonts, name)
        if name == "texture" then
            ensure_texture_loaded()
            return texture
        end
        local entry = data[name]
        if not entry then
            return nil
        end
        ensure_texture_loaded()
        entry.texture = texture
        return entry
    end})
    return fonts
end
