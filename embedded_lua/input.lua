-- keyboard

amulet.key_down = {}
amulet.key_pressed = {}
amulet.key_released = {}

function amulet._key_down(key)
    amulet.key_down[key] = true
    amulet.key_pressed[key] = (amulet.key_pressed[key] or 0) + 1
end

function amulet._key_up(key)
    amulet.key_down[key] = nil
    amulet.key_released[key] = (amulet.key_released[key] or 0) + 1
end

local
function clear_keys()
    table.clear(amulet.key_pressed)    
    table.clear(amulet.key_released)    
end

-- mouse

amulet.mouse_position = vec2(0, 0)
amulet.mouse_button_down = {}
amulet.mouse_button_pressed = {}
amulet.mouse_button_released = {}

function amulet._mouse_down(button)
    amulet.mouse_button_down[button] = true
    amulet.mouse_button_pressed[button] = (amulet.mouse_button_pressed[button] or 0) + 1
end

function amulet._mouse_up(button)
    amulet.mouse_button_down[button] = nil
    amulet.mouse_button_released[button] = (amulet.mouse_button_released[button] or 0) + 1
end

function amulet._mouse_move(x, y)
    amulet.mouse_position.x = x
    amulet.mouse_position.y = y
end

local
function clear_mouse()
    table.clear(amulet.mouse_button_pressed)    
    table.clear(amulet.mouse_button_released)    
end

-- general

function amulet._clear_input()
    clear_keys()
    clear_mouse()
end
