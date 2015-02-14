function amulet.float_array(values)
    local view = amulet.buffer(4 * #values):view("float", 0, 4)
    view:set_nums(values)
    return view
end

function amulet.ushort_elem_array(values)
    local view = amulet.buffer(2 * #values):view("ushort_elem", 0, 2)
    view:set_nums(values)
    return view
end

function amulet.uint_elem_array(values)
    local view = amulet.buffer(4 * #values):view("uint_elem", 0, 4)
    view:set_nums(values)
    return view
end
