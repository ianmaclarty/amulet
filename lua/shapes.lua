-- rects:
-- 1-4
-- |\|
-- 2-3

function amulet.rect_indices() 
    return amulet.ushort_elem_array{1, 2, 3, 1, 3, 4}
end

function amulet.rect_verts_2d(x1, y1, x2, y2)
    return amulet.vec2_array{x1, y2, x1, y1, x2, y1, x2, y2}
end

function amulet.rect_verts_3d(x1, y1, x2, y2)
    return amulet.vec3_array{x1, y2, 0, x1, y1, 0, x2, y1, 0, x2, y2, 0}
end
