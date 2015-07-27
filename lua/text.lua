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

local
function set_text_verts(font, str, verts_view, uvs_view)
    local x = 0
    local y = 0
    local verts = {}
    local uvs = {}
    local v = 1
    local u = 1
    for p, c in utf8.codes(str) do
        local char_data = font[c]
        local tex_coords = char_data.tex_coords
        local x1, y1, x2, y2 = char_data.x1, char_data.y1, char_data.x2, char_data.y2
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
        uvs[u]     = tex_coords[1]
        uvs[u + 1] = tex_coords[2]
        uvs[u + 2] = tex_coords[3]
        uvs[u + 3] = tex_coords[4]
        uvs[u + 4] = tex_coords[5]
        uvs[u + 5] = tex_coords[6]
        uvs[u + 6] = tex_coords[7]
        uvs[u + 7] = tex_coords[8]
        u = u + 8
        x = x + char_data.advance
    end
    verts_view:set(verts)
    uvs_view:set(uvs)
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
