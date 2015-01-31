function amulet.float_array(values)
    local n = #values
    local view = amulet.buffer(4 * n):view("float", 0, 4)
    view:set_nums(values)
    return view
end
