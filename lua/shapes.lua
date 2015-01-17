function amulet.rect(x1, y1, x2, y2)
    local pos_view = amulet.buffer(8*4):view("vec2", 0, 8)
    pos_view:set_nums{
        x1, y1,
        x2, y1,
        x2, y2,
        x1, y2,
    }
    return amulet.draw_arrays("triangle_fan"):bind_array("pos", pos_view)
end
