local
function make_buffer(num_verts)
    local buffer = am.buffer(num_verts * 16)
    local verts = buffer:view("vec2", 0, 16)
    local uvs = buffer:view("vec2", 8, 16)
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
    local max_width = 0
    local h = font.line_height
    local row = 1
    for p, c in utf8.codes(str) do
        if c == newline then
            h = h + font.line_height
            row = row + 1
            row_widths[row] = 0
        else
            local char_data = chars[c] or chars[0] or chars[space]
            local w = row_widths[row] + char_data.advance
            row_widths[row] = w
            if w > max_width then
                max_width = w
            end
        end
    end
    
    row = 1
    if halign == "center" then
        x = -math.floor(row_widths[row] / 2)
    elseif halign == "right" then
        x = -row_widths[row]
    else
        x = 0
    end
    if valign == "center" then
        y = math.floor(h / 2) - font.line_height
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
                x = -math.floor(row_widths[row] / 2)
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
        verts[v + 2] = x + x1
        verts[v + 3] = y + y1
        verts[v + 4] = x + x2
        verts[v + 5] = y + y1
        verts[v + 6] = x + x2
        verts[v + 7] = y + y2
        v = v + 8
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

    return max_width, h
end

local
function set_sprite_verts(img, verts_view, uvs_view, halign, valign)
    local x1, y1, x2, y2 = img.x1, img.y1, img.x2, img.y2
    local s1, t1, s2, t2 = img.s1, img.t1, img.s2, img.t2
    local w = img.width
    local h = img.height
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
    verts_view:set{x1 + x_os, y2 + y_os, x1 + x_os, y1 + y_os, x2 + x_os, y1 + y_os, x2 + x_os, y2 + y_os}
    uvs_view:set{s1, t2, s1, t1, s2, t1, s2, t2}
end

local default_font_key = [[
 ☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼
 !"#$%&'()*+,-./0123456789:;<=>?
@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_
`abcdefghijklmnopqrstuvwxyz{|}~⌂
ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒ
áíóúñÑªº¿⌐¬½¼¡«»░▒▓│┤╡╢╖╕╣║╗╝╜╛┐
└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀
αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■ 
]]

function am.text(font, str, color, halign, valign)
    if type(font) ~= "table" then
        str, color, halign, valign = font, str, color, halign
        if not am.default_font then
            am.default_font = am.load_bitmap_font("lua/default_font.png", default_font_key, true)
        end
        font = am.default_font
    end
    if type(color) == "string" then
        halign, valign = color, halign
        color = vec4(1)
    end
    halign = halign or "center"
    valign = valign or "center"
    color = color or vec4(1)
    if not font.is_font then
        error("expecting a font in position 1", 2)
    end
    str = type(str) == "string" and str or tostring(str)
    if str == "" then
        -- zero length strings cause issues
        str = " "
    end
    local len = utf8.len(str)
    local num_verts = len * 4
    local buffer, verts, uvs = make_buffer(num_verts)
    local indices = make_indices(num_verts)
    local w, h = set_text_verts(font, str, verts, uvs, halign, valign)
    local node =
        am.blend(font.is_premult and "premult" or "alpha")
        ^am.use_program(font.is_premult and am.shaders.premult_texturecolor2d or am.shaders.texturecolor2d)
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
        w, h = set_text_verts(font, str, verts, uvs, halign, valign)
        self"draw".count = utf8.len(str) * 6
    end
    function node:get_color()
        return color
    end
    function node:set_color(col)
        color = col
        self"bind".color = color
    end
    function node:get_width()
        return w
    end
    function node:get_height()
        return h
    end
    node:tag"text"
    return node
end

local sprite_cache = {}

am.ascii_color_map = {
    W = vec4(1, 1, 1, 1),
    w = vec4(0.75, 0.75, 0.75, 1),
    K = vec4(0, 0, 0, 1),
    k = vec4(0.5, 0.5, 0.5, 1),
    R = vec4(1, 0, 0, 1),
    r = vec4(0.5, 0, 0, 1),
    Y = vec4(1, 1, 0, 1),
    y = vec4(0.5, 0.5, 0, 1),
    G = vec4(0, 1, 0, 1),
    g = vec4(0, 0.5, 0, 1),
    C = vec4(0, 1, 1, 1),
    c = vec4(0, 0.5, 0.5, 1),
    B = vec4(0, 0, 1, 1),
    b = vec4(0, 0, 0.5, 1),
    M = vec4(1, 0, 1, 1),
    m = vec4(0.5, 0, 0.5, 1),
    O = vec4(1, 0.5, 0, 1),
    o = vec4(0.5, 0.25, 0, 1),
}

local
function parse_ascii_sprite(str)
    local key = am.ascii_color_map
    local lf = string.byte("\n")
    local cr = string.byte("\r")
    local sp = string.byte(" ")
    local tb = string.byte("\t")
    local width = 0
    local height
    local curr_width = 0
    local colors = {}
    local row = 1
    local rkey = {}
    local gkey = {}
    local bkey = {}
    local akey = {}
    for chr, color in pairs(key) do
        local bt = chr:byte()
        rkey[bt] = color.r * 255
        gkey[bt] = color.g * 255
        bkey[bt] = color.b * 255
        akey[bt] = color.a * 255
    end
    for pos = 1, #str do
        local b = str:byte(pos)
        if b == cr or b == sp or b == tb then
            -- skip 
        elseif b == lf then
            if row == 1 then
                width = curr_width
            elseif row > 1 and curr_width ~= width then
                error("all rows must have the same width", 4)
            end
            row = row + 1
            curr_width = 0
        else
            if not colors[row] then
                colors[row] = {}
            end
            table.insert(colors[row], b)
            curr_width = curr_width + 1
        end
    end
    if curr_width > 0 then
        if curr_width ~= width then
            error("all rows must have the same width", 4)
        end
        height = row
    else
        height = row - 1
    end

    local buf = am.buffer(4 * width * height)
    local rview = buf:view("ubyte", 0, 4)
    local gview = buf:view("ubyte", 1, 4)
    local bview = buf:view("ubyte", 2, 4)
    local aview = buf:view("ubyte", 3, 4)
    local i = 1
    for row = height, 1, -1 do
        for col = 1, width do
            local bt = colors[row][col]
            rview[i] = rkey[bt] or 0
            gview[i] = gkey[bt] or 0
            bview[i] = bkey[bt] or 0
            aview[i] = akey[bt] or 0
            i = i + 1
        end
    end

    local tex = am.texture2d(am.image_buffer(buf, width, height))

    return {
        texture = tex,
        s1 = 0, t1 = 0, s2 = 1, t2 = 1,
        x1 = 0, y1 = 0, x2 = width, y2 = height,
        width = width, height = height,
    }
end

local
function convert_sprite_source(img)
    local t = type(img)
    if t == "table" then
        return img
    elseif t == "string" then
        local name = img
        if sprite_cache[name] then
            return sprite_cache[name]
        end
        local sprite
        if img:match("%\n") then
            sprite = parse_ascii_sprite(img)
        else
            local texture = am.texture2d(img)
            local s1, t1, s2, t2 = 0, 0, 1, 1
            local x1, y1, x2, y2 = 0, 0, texture.width, texture.height
            sprite = {
                texture = texture,
                s1 = s1,
                t1 = t1,
                s2 = s2,
                t2 = t2,
                x1 = x1,
                y1 = y1,
                x2 = x2,
                y2 = y2,
                width = x2 - x1,
                height = y2 - y1,
            }
        end
        sprite_cache[name] = sprite
        return sprite
    else
        error("unexpected "..t.." in argument position 1", 3)
    end
end

am._convert_sprite_source = convert_sprite_source

function am.sprite(image0, color, halign, valign)
    if type(color) == "string" then
        halign, valign = color, halign
        color = vec4(1)
    end
    halign = halign or "center"
    valign = valign or "center"
    color = color or vec4(1)
    local image = convert_sprite_source(image0)
    local num_verts = 4
    local buffer, verts, uvs = make_buffer(num_verts)
    local indices = make_indices(num_verts)
    set_sprite_verts(image, verts, uvs, halign, valign)
    local node =
        am.blend(image.is_premult and "premult" or "alpha")
        ^am.use_program(image.is_premult and am.shaders.premult_texturecolor2d or am.shaders.texturecolor2d)
        ^am.bind{
            vert = verts,
            uv = uvs,
            tex = image.texture,
            color = color,
        }
        ^am.draw("triangles", indices)
    function node:get_source()
        return image0
    end
    function node:set_source(img0)
        image = convert_sprite_source(img0)
        image0 = img0
        set_sprite_verts(image, verts, uvs, halign, valign)
        self"bind".tex = image.texture
    end
    function node:get_color()
        return color
    end
    function node:set_color(c)
        color = c
        self"bind".color = color
    end
    function node:get_width()
        return image.width
    end
    function node:get_height()
        return image.height
    end
    function node:get_spec()
        return image
    end 
        
    node:tag"sprite"
    return node
end

function am._init_fonts(data, imgfile, embedded)
    local fonts = {}
    for _, entry in ipairs(data) do
        local name = entry.filename:match("([^/:\\]+)%.([^/%.:\\]+)$")
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
        entry.is_premult = data.is_premult
        data[name] = entry
    end
    local texture
    local
    function ensure_texture_loaded()
        if not texture then
            if embedded then
                texture = am.texture2d(am._load_embedded_image(imgfile));
            else
                texture = am.texture2d(imgfile);
            end
            texture.minfilter = data.minfilter
            texture.magfilter = data.magfilter
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

function am.load_sprite_grid(filename, width, height)
    local tex = am.texture2d(filename)
    local num_rows, num_cols = math.floor(tex.height / height), math.floor(tex.width / width)
    local grid = {}
    local w, h = tex.width, tex.height
    local dx = w / num_cols
    local dy = h / num_rows
    local ds = 1 / num_cols
    local dt = 1 / num_rows
    for row = 1, num_rows do
        grid[row] = {}
        for col = 1, num_cols do
            grid[row][col] = {
                x1 = 0,
                y1 = 0,
                x2 = dx,
                y2 = dy,
                s1 = (col - 1) * ds,
                t1 = (num_rows - row) * dt,
                s2 = col * ds,
                t2 = (num_rows - row + 1) * dt,
                width = dx,
                height = dy,
                texture = tex,
            }
        end
    end
    return grid
end

function am.load_bitmap_font(filename, key, embedded)
    local texture
    if embedded then
        texture = am.texture2d(am._load_embedded_image(filename))
    else
        texture = am.texture2d(filename)
    end
    local num_rows, num_cols = 0, 0
    local chars = {}
    local row, col = 1, 1
    for p, c in utf8.codes(key) do
        if c ~= newline then
            chars[c] = {row = row, col = col}
            col = col + 1
        else
            num_cols = math.max(col - 1, num_cols)
            col = 1
            row = row + 1
        end
    end
    num_cols = math.max(col - 1, num_cols)
    num_rows = col == 1 and row - 1 or row
    local w, h = texture.width, texture.height
    local dx = w / num_cols
    local dy = h / num_rows
    local ds = 1 / num_cols
    local dt = 1 / num_rows
    for c, info in pairs(chars) do
        local i, j = info.col - 1, num_rows - info.row
        info.x1 = 0
        info.y1 = 0
        info.x2 = dx
        info.y2 = dy
        info.s1 = i * ds
        info.t1 = j * dt
        info.s2 = info.s1 + ds
        info.t2 = info.t1 + dt
        info.advance = dx
    end
    local font = {
        line_height = dy,
        is_premult = false,
        is_font = true,
        chars = chars,
        texture = texture,
    }
    return font
end
