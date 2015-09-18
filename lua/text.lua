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
function set_text_verts(font, str, verts_view, uvs_view, halign, valign)
    local x = 0
    local y = 0
    local verts = {}
    local uvs = {}
    local v = 1
    local u = 1
    local chars = font.chars

    -- compute row_widths and height
    local row_widths = {0}
    local h = font.line_height
    local row = 1
    for p, c in utf8.codes(str) do
        if c == newline then
            h = h + font.line_height
            row = row + 1
            row_widths[row] = 0
        else
            local char_data = chars[c] or chars[0] or chars[space]
            row_widths[row] = row_widths[row] + char_data.advance
        end
    end
    
    row = 1
    if halign == "center" then
        x = -row_widths[row] / 2
    elseif halign == "right" then
        x = -row_widths[row]
    else
        x = 0
    end
    if valign == "center" then
        y = (h - font.line_height) / 2
    elseif valign == "bottom" then
        y = h - font.line_height
    else
        y = -font.line_height
    end
    for p, c in utf8.codes(str) do
        local char_data, x1, y1, x2, y2, advance
        if c == newline then
            y = y - font.line_height
            x1, y1, x2, y2 = 0, 0, 0, 0
            char_data = chars[space]
            advance = 0
            row = row + 1
            if halign == "center" then
                x = -row_widths[row] / 2
            elseif halign == "right" then
                x = -row_widths[row]
            else
                x = 0
            end
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
    uvs_view:set(img.uvs)
end

function am.text(font, str, color, halign, valign)
    if type(font) == "string" then
        str, color, halign, valign = font, str, halign, color
        font = am.default_font.default16
    end
    if type(color) ~= "userdata" then
        halign, valign = color, halign
        color = vec4(1)
    end
    halign = halign or "center"
    valign = valign or "center"
    if not font.is_font then
        error("expecting a font in position 1", 2)
    end
    str = type(str) == "string" and str or tostring(str)
    local len = utf8.len(str)
    local num_verts = len * 4
    local buffer, verts, uvs = make_buffer(num_verts)
    local indices = make_indices(num_verts)
    set_text_verts(font, str, verts, uvs, halign, valign)
    local node =
        am.blend("premult")
        ^am.use_program(am.shaders.texturecolor)
        ^am.bind{
            vert = verts,
            uv = uvs,
            tex = font.texture,
            color = color,
        }
        ^am.draw("triangles", indices)
    function node:get_text()
        return str
    end
    function node:set_text(str1)
        str1 = type(str1) == "string" and str1 or tostring(str1)
        local len1 = utf8.len(str1)
        if len1 > len then
            local num_verts = len1 * 4 
            buffer, verts, uvs = make_buffer(num_verts)
            indices = make_indices(num_verts)
            self"bind".vert = verts
            self"bind".uv = uvs
            self"draw".elements = indices
            len = len1
        end
        str = str1
        set_text_verts(font, str, verts, uvs, halign, valign)
        self"draw".count = utf8.len(str) * 6
    end
    function node:get_color()
        return color
    end
    function node:set_color(col)
        color = col
        self"bind".color = color
    end
    return node
end

function am.sprite(image, halign, valign)
    halign = halign or "center"
    valign = valign or "center"
    local num_verts = 4
    local buffer, verts, uvs = make_buffer(num_verts)
    local indices = make_indices(num_verts)
    set_sprite_verts(image, verts, uvs, halign, valign)
    local node =
        am.blend("premult")
        ^am.use_program(am.shaders.texture)
        ^am.bind{
            vert = verts,
            uv = uvs,
            tex = image.texture,
        }
        ^am.draw("triangles", indices)
    function node:get_sprite()
        return image
    end
    function node:set_sprite(img)
        image = img
        set_sprite_verts(image, verts, uvs, halign, valign)
    end
    return node
end

function am._init_fonts(data, imgfile, embedded)
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
            local img
            if embedded then
                img = am._load_embedded_image(imgfile);
            else
                img = am.load_image(imgfile);
            end
            texture = am.texture2d{buffer = img.buffer, width = img.width, height = img.height,
                minfilter = data.minfilter, magfilter = data.magfilter}
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
