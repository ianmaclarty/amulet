local window_mt = _metatable_registry.window

-- keyboard

function amulet._key_down(win, key)
    local u = getusertable(win)
    u._key_down[key] = true
    u._key_pressed[key] = (u._key_pressed[key] or 0) + 1
end

function amulet._key_up(win, key)
    local u = getusertable(win)
    u._key_down[key] = nil
    u._key_released[key] = (u._key_released[key] or 0) + 1
end

local
function clear_keys(win)
    local u = getusertable(win)
    table.clear(u._key_pressed)    
    table.clear(u._key_released)    
end

function window_mt.key_down(win, key)
    local u = getusertable(win)
    return u._key_down[key]
end

function window_mt.key_pressed(win, key)
    local u = getusertable(win)
    return u._key_pressed[key]
end

function window_mt.key_released(win, key)
    local u = getusertable(win)
    return u._key_released[key]
end

-- mouse

function amulet._mouse_down(win, button)
    local u = getusertable(win)
    u._mouse_button_down[button] = true
    u._mouse_button_pressed[button] = (u._mouse_button_pressed[button] or 0) + 1
end

function amulet._mouse_up(win, button)
    local u = getusertable(win)
    u._mouse_button_down[button] = nil
    u._mouse_button_released[button] = (u._mouse_button_released[button] or 0) + 1
end

function amulet._mouse_move(win, x, y)
    local u = getusertable(win)
    u._mouse_position.x = x
    u._mouse_position.y = y
    u._mouse_delta.x = u._mouse_position.x - u._prev_mouse_position.x
    u._mouse_delta.y = u._mouse_position.y - u._prev_mouse_position.y
end

local
function clear_mouse(win)
    local u = getusertable(win)
    table.clear(u._mouse_button_pressed)    
    table.clear(u._mouse_button_released)    
    u._prev_mouse_position.x = u._mouse_position.x
    u._prev_mouse_position.y = u._mouse_position.y
end

function window_mt.mouse_button_down(win, button)
    local u = getusertable(win)
    return u._mouse_button_down[button]
end

function window_mt.mouse_button_pressed(win, button)
    local u = getusertable(win)
    return u._mouse_button_pressed[button]
end

function window_mt.mouse_button_released(win, button)
    local u = getusertable(win)
    return u._mouse_button_released[button]
end

function window_mt.mouse_position(win)
    local u = getusertable(win)
    return u._mouse_position
end

function window_mt.mouse_delta(win)
    local u = getusertable(win)
    return u._mouse_delta
end

-- touch

function amulet._touch_begin(win, id, x, y)
    local u = getusertable(win)
    local free_i = nil
    for i, touch in ipairs(u._touches) do
        if touch.id == id then
            free_i = i
            break
        elseif not touch.id and not free_i then
            free_i = i
        end
    end
    if free_i then
        local touch = u._touches[free_i]
        touch.id = id
        touch.pos.x = x
        touch.pos.y = y
        u._touch_began[free_i] = (u._touch_began[free_i] or 0) + 1
    end
end

function amulet._touch_end(win, id, x, y)
    local u = getusertable(win)
    for i, touch in ipairs(u._touches) do
        if touch.id == id then
            touch.id = nil
            u._touch_ended[i] = (u._touch_ended[i] or 0) + 1
            return
        end
    end
end

function amulet._touch_move(win, id, x, y)
    local u = getusertable(win)
    for i, touch in ipairs(u._touches) do
        if touch.id == id then
            touch.pos.x = x
            touch.pos.y = y
            return
        end
    end
end

function window_mt.touch_pos(win, i)
    local u = getusertable(win)
    local touch = u._touches[i]
    if not touch then
        return nil
    else
        if touch.id then
            return touch.pos
        else
            return nil
        end
    end
end

function window_mt.touch_began(win, i)
    local u = getusertable(win)
    return u._touch_began[i]
end

function window_mt.touch_ended(win, i)
    local u = getusertable(win)
    return u._touch_ended[i]
end

local
function clear_touch(win)
    local u = getusertable(win)
    table.clear(u._touch_began)
    table.clear(u._touch_ended)
end

function amulet._clear_events(win)
    clear_keys(win)
    clear_mouse(win)
    clear_touch(win)
end

function amulet._init_window_event_data(window)
    local u = getusertable(window)

    -- keyboard

    u._key_down = {}
    u._key_pressed = {}
    u._key_released = {}

    -- mouse

    u._prev_mouse_position = vec2(0, 0)
    u._mouse_position = vec2(0, 0)
    u._mouse_delta = vec2(0, 0)
    u._mouse_button_down = {}
    u._mouse_button_pressed = {}
    u._mouse_button_released = {}

    -- touch

    local max_touches = 10
    u._touches = {}
    u._touch_began = {}
    u._touch_ended = {}

    for i = 1, max_touches do
        u._touches[i] = {pos = vec2(0)}
    end
end
