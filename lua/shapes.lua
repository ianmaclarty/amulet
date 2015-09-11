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

local rect_props = {}
for field, os in pairs(offsets) do
    local os1, os2, c = unpack(os)
    rect_props["get_"..field] = function(node)
        return node.verts[os1][c]
    end
    rect_props["set_"..field] = function(node, val)
        local verts = node.verts
        verts[os1] = verts[os1](c, val)
        verts[os2] = verts[os2](c, val)
    end
end
function rect_props:get_color()
    return self"bind".color
end
function rect_props:set_color(color)
    self"bind".color = color
end

function am.rect(x1, y1, x2, y2, color)
    color = color or vec4(1)
    local verts = am.rect_verts_3d(x1, y1, x2, y2)
    local node = am.use_program(am.shaders.color)
        ^am.bind{
            pos = verts,
            color = color,
        }
        ^am.draw_elements(am.rect_indices())
    node.verts = verts
    for prop, func in pairs(rect_props) do
        node[prop] = func
    end
    return node
end
