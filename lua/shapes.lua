local am = amulet

-- rects:
-- 1-4
-- |\|
-- 2-3

function am.rect_indices() 
    return am.ushort_elem_array{1, 2, 3, 1, 3, 4}
end

function am.rect_verts_2d(x1, y1, x2, y2)
    return am.vec2_array{x1, y2, x1, y1, x2, y1, x2, y2}
end

function am.rect_verts_3d(x1, y1, x2, y2)
    return am.vec3_array{x1, y2, 0, x1, y1, 0, x2, y1, 0, x2, y2, 0}
end

local offsets = {
    x1 = {1, 2, 1},
    y1 = {2, 3, 2},
    x2 = {3, 4, 1},
    y2 = {1, 4, 2},
}

local rect_ext = {}
for field, os in pairs(offsets) do
    local os1, os2, c = unpack(os)
    rect_ext["get_"..field] = function(node)
        return node.verts[os1][c]
    end
    rect_ext["set_"..field] = function(node, val)
        local verts = node.verts
        local v = verts[os1]
        v[c] = val
        verts[os1] = v
        v = verts[os2]
        v[c] = val
        verts[os2] = v
    end
end
function rect_ext.get_color(node)
    return node.col.rgba
end
function rect_ext.set_color(node, val)
    node.col.rgba = val
end

function am.rect(x1, y1, x2, y2, color)
    color = color or vec4(1)
    local verts = am.rect_verts_3d(x1, y1, x2, y2)
    local node = am.draw_elements(am.rect_indices())
        :bind_array("pos", verts)
        :bind_vec4("color", color):alias("col")
        :bind_program(am.shaders.color)
        :alias("verts", verts)
        :extend(rect_ext)
    return node
end
