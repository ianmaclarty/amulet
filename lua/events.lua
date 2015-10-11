local window_mt = _metatable_registry.window

-- keyboard

function am._key_down(win, key)
    local u = win._event_data
    u._key_down[key] = true
    u._key_pressed[key] = (u._key_pressed[key] or 0) + 1
end

function am._key_up(win, key)
    local u = win._event_data
    u._key_down[key] = nil
    u._key_released[key] = (u._key_released[key] or 0) + 1
end

local
function clear_keys(win)
    local u = win._event_data
    table.clear(u._key_pressed)    
    table.clear(u._key_released)    
end

function window_mt.key_down(win, key)
    local u = win._event_data
    return u._key_down[key]
end

function window_mt.key_pressed(win, key)
    local u = win._event_data
    return u._key_pressed[key]
end

function window_mt.key_released(win, key)
    local u = win._event_data
    return u._key_released[key]
end

-- mouse

function am._mouse_down(win, button)
    local u = win._event_data
    u._mouse_down[button] = true
    u._mouse_pressed[button] = (u._mouse_pressed[button] or 0) + 1
end

function am._mouse_up(win, button)
    local u = win._event_data
    u._mouse_down[button] = nil
    u._mouse_released[button] = (u._mouse_released[button] or 0) + 1
end

function am._mouse_move(win, x, y)
    local u = win._event_data
    u._mouse_position = vec2(x, y)
    u._mouse_delta = vec2(u._mouse_position - u._prev_mouse_position)
end

local
function clear_mouse(win)
    local u = win._event_data
    table.clear(u._mouse_pressed)    
    table.clear(u._mouse_released)    
    u._prev_mouse_position = u._mouse_position
end

function window_mt.mouse_down(win, button)
    local u = win._event_data
    return u._mouse_down[button]
end

function window_mt.mouse_pressed(win, button)
    local u = win._event_data
    return u._mouse_pressed[button]
end

function window_mt.mouse_released(win, button)
    local u = win._event_data
    return u._mouse_released[button]
end

function window_mt.mouse_position(win)
    local u = win._event_data
    return u._mouse_position
end

function window_mt.mouse_position_norm(win)
    local u = win._event_data
    local sz = vec2(win.width, win.height)
    return (u._mouse_position * 2 - sz) / sz
end

function window_mt.mouse_pixel_position(win)
    local u = win._event_data
    local sz = vec2(win.width, win.height)
    return (u._mouse_position - vec2(win.left, win.bottom)) / sz * vec2(win.pixel_width, win.pixel_height)
end

function window_mt.mouse_delta(win)
    local u = win._event_data
    return u._mouse_delta
end

function window_mt.mouse_delta_norm(win)
    local u = win._event_data
    local sz = vec2(win.width, win.height)
    return u._mouse_delta * 2 / sz
end

function window_mt.mouse_pixel_delta(win)
    local u = win._event_data
    local sz = vec2(win.width, win.height)
    return u._mouse_delta / sz * vec2(win.pixel_width, win.pixel_height)
end

-- touch

function am._touch_begin(win, id, x, y)
    local u = win._event_data
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

function am._touch_end(win, id, x, y)
    local u = win._event_data
    for i, touch in ipairs(u._touches) do
        if touch.id == id then
            touch.id = nil
            u._touch_ended[i] = (u._touch_ended[i] or 0) + 1
            return
        end
    end
end

function am._touch_move(win, id, x, y)
    local u = win._event_data
    for i, touch in ipairs(u._touches) do
        if touch.id == id then
            touch.pos.x = x
            touch.pos.y = y
            return
        end
    end
end

function window_mt.touch_position(win, i)
    local u = win._event_data
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
    local u = win._event_data
    return u._touch_began[i]
end

function window_mt.touch_ended(win, i)
    local u = win._event_data
    return u._touch_ended[i]
end

local
function clear_touch(win)
    local u = win._event_data
    table.clear(u._touch_began)
    table.clear(u._touch_ended)
end

-- resize

local
function resized(win)
    local u = win._event_data
    return u._prev_width ~= win.pixel_width or u._prev_height ~= win.pixel_height
end

local
function clear_resized(win)
    local u = win._event_data
    u._prev_width = win.pixel_width
    u._prev_height = win.pixel_height
end

-- setup

function am._clear_events(win)
    clear_keys(win)
    clear_mouse(win)
    clear_touch(win)
    clear_resized(win)
end

function am._init_window_event_data(window)
    local u = window._event_data

    -- keyboard

    u._key_down = {}
    u._key_pressed = {}
    u._key_released = {}

    -- mouse

    u._prev_mouse_position = vec2(0, 0)
    u._mouse_position = vec2(0, 0)
    u._mouse_delta = vec2(0, 0)
    u._mouse_down = {}
    u._mouse_pressed = {}
    u._mouse_released = {}

    -- touch

    local max_touches = 10
    u._touches = {}
    u._touch_began = {}
    u._touch_ended = {}

    for i = 1, max_touches do
        u._touches[i] = {pos = vec2(0)}
    end

    -- resize

    u._prev_height = 0
    u._prev_width = 0
    window.resized = resized

    if not am._main_window then
        am._main_window = window
    end
end
