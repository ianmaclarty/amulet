amulet.key_down = {}
amulet.key_pressed = {}
amulet.key_released = {}

function amulet._key_down(key)
    amulet.key_pressed[key] = (amulet.key_pressed[key] or 0) + 1
    amulet.key_down[key] = true
end

function amulet._key_up(key)
    amulet.key_released[key] = (amulet.key_released[key] or 0) + 1
    amulet.key_down[key] = nil
end

function amulet._clear_keys()
    table.clear(amulet.key_pressed)    
    table.clear(amulet.key_released)    
end
