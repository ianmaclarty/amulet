local window_mt = _metatable_registry.window

-- keyboard

function am._key_down(win, key)
    local u = win._event_data
    u._key_presses[key] = (u._key_presses[key] or 0) + 1
    --log("down: "..key.."("..u._key_presses[key]..", "..tostring(u._key_state0[key])..")")
end

function am._key_up(win, key)
    local u = win._event_data
    u._key_releases[key] = (u._key_releases[key] or 0) + 1
    --log("up: "..key.."("..u._key_releases[key]..", "..tostring(u._key_state0[key])..")")
end

local
function clear_keys(win)
    local u = win._event_data
    for key, presses in pairs(u._key_presses) do
        local state0 = u._key_state0[key]
        local releases = u._key_releases[key] or 0
        local new_state = not state0 and presses > 0 or state0 and releases == 0
        local total = (state0 and 1 or 0) + presses - releases
        presses = 0
        releases = 0
        if total <= 0 and new_state then
            -- postpose release to next frame
            releases = 1
        elseif total > 0 and not new_state then
            -- postpone press to next frame
            presses = 1
        end
        u._key_state0[key] = new_state
        u._key_presses[key] = presses
        u._key_releases[key] = releases
    end
end

function window_mt.key_down(win, key)
    key = tostring(key)
    local u = win._event_data
    local state0 = u._key_state0[key]
    local presses = u._key_presses[key] or 0
    local releases = u._key_releases[key] or 0
    return not state0 and presses > 0 or state0 and releases <= 0
end

function window_mt.keys_down(win)
    local u = win._event_data
    local keys = {}
    for key, _ in pairs(u._key_presses) do
        if win:key_down(key) then
            table.insert(keys, key)
        end
    end
    return keys
end

function window_mt.key_pressed(win, key)
    key = tostring(key)
    local u = win._event_data
    return not u._key_state0[key] and (u._key_presses[key] or 0) > 0
end

function window_mt.key_released(win, key)
    key = tostring(key)
    local u = win._event_data
    return u._key_state0[key] and (u._key_releases[key] or 0) > 0
end

-- mouse

function am._mouse_down(win, button)
    local u = win._event_data
    u._mouse_presses[button] = (u._mouse_presses[button] or 0) + 1
end

function am._mouse_up(win, button)
    local u = win._event_data
    u._mouse_releases[button] = (u._mouse_releases[button] or 0) + 1
end

function am._mouse_move(win, x, y, nx, ny, px, py)
    local u = win._event_data
    u._mouse_position_x = x
    u._mouse_position_y = y
    u._mouse_norm_position_x = nx
    u._mouse_norm_position_y = ny
    u._mouse_pixel_position_x = px
    u._mouse_pixel_position_y = py
end

function am._mouse_wheel(win, x, y)
    local u = win._event_data
    u._mouse_wheel_x = x
    u._mouse_wheel_y = y
end

local
function clear_mouse(win)
    local u = win._event_data
    for button, presses in pairs(u._mouse_presses) do
        local state0 = u._mouse_state0[button]
        local releases = u._mouse_releases[button] or 0
        local new_state = not state0 and presses > 0 or state0 and releases == 0
        local total = (state0 and 1 or 0) + presses - releases
        presses = 0
        releases = 0
        if total <= 0 and new_state then
            -- postpose release to next frame
            releases = 1
        elseif total > 0 and not new_state then
            -- postpone press to next frame
            presses = 1
        end
        u._mouse_state0[button] = new_state
        u._mouse_presses[button] = presses
        u._mouse_releases[button] = releases
    end
    u._prev_mouse_position_x = u._mouse_position_x
    u._prev_mouse_position_y = u._mouse_position_y
    u._prev_mouse_norm_position_x = u._mouse_norm_position_x
    u._prev_mouse_norm_position_y = u._mouse_norm_position_y
    u._prev_mouse_pixel_position_x = u._mouse_pixel_position_x
    u._prev_mouse_pixel_position_y = u._mouse_pixel_position_y
    u._prev_mouse_wheel_x = u._mouse_wheel_x
    u._prev_mouse_wheel_y = u._mouse_wheel_y
end

function window_mt.mouse_down(win, button)
    local u = win._event_data
    local state0 = u._mouse_state0[button]
    local presses = u._mouse_presses[button] or 0
    local releases = u._mouse_releases[button] or 0
    return not state0 and presses > 0 or state0 and releases <= 0
end

function window_mt.mouse_pressed(win, button)
    local u = win._event_data
    return not u._mouse_state0[button] and (u._mouse_presses[button] or 0) > 0
end

function window_mt.mouse_released(win, button)
    local u = win._event_data
    return u._mouse_state0[button] and (u._mouse_releases[button] or 0) > 0
end

function window_mt.mouse_position(win)
    local u = win._event_data
    return vec2(u._mouse_position_x, u._mouse_position_y)
end

function window_mt.mouse_norm_position(win)
    local u = win._event_data
    return vec2(u._mouse_norm_position_x, u._mouse_norm_position_y)
end

function window_mt.mouse_pixel_position(win)
    local u = win._event_data
    return vec2(u._mouse_pixel_position_x, u._mouse_pixel_position_y)
end

function window_mt.mouse_delta(win)
    local u = win._event_data
    return vec2(u._mouse_position_x - u._prev_mouse_position_x, u._mouse_position_y - u._prev_mouse_position_y)
end

function window_mt.mouse_norm_delta(win)
    local u = win._event_data
    return vec2(u._mouse_norm_position_x - u._prev_mouse_norm_position_x, u._mouse_norm_position_y - u._prev_mouse_norm_position_y)
end

function window_mt.mouse_pixel_delta(win)
    local u = win._event_data
    return vec2(u._mouse_pixel_position_x - u._prev_mouse_pixel_position_x, u._mouse_pixel_position_y - u._prev_mouse_pixel_position_y)
end

function window_mt.mouse_wheel(win)
    local u = win._event_data
    return vec2(u._mouse_wheel_x, u._mouse_wheel_y)
end

function window_mt.mouse_wheel_delta(win)
    local u = win._event_data
    return vec2(u._mouse_wheel_x - u._prev_mouse_wheel_x, u._mouse_wheel_y - u._prev_mouse_wheel_y)
end

-- touch

function am._touch_begin(win, id, x, y, nx, ny, px, py)
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
        touch.position_x = x
        touch.position_y = y
        touch.norm_position_x = x
        touch.norm_position_y = y
        touch.pixel_position_x = x
        touch.pixel_position_y = y
        touch.began = true
    end
end

function am._touch_end(win, id, x, y, nx, ny, px, py)
    local u = win._event_data
    for i, touch in ipairs(u._touches) do
        if touch.id == id then
            touch.position_x = x
            touch.position_y = y
            touch.norm_position_x = x
            touch.norm_position_y = y
            touch.pixel_position_x = x
            touch.pixel_position_y = y
            touch.ended = true
            return
        end
    end
end

function am._touch_move(win, id, x, y)
    local u = win._event_data
    for i, touch in ipairs(u._touches) do
        if touch.id == id then
            touch.position_x = x
            touch.position_y = y
            touch.norm_position_x = x
            touch.norm_position_y = y
            touch.pixel_position_x = x
            touch.pixel_position_y = y
            return
        end
    end
end

function window_mt.touch_position(win, i)
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return vec2(0, 0)
    else
        return vec2(touch.position_x, touch.position_y)
    end
end

function window_mt.touch_norm_position(win, i)
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return vec2(0, 0)
    else
        return vec2(touch.norm_position_x, touch.norm_position_y)
    end
end

function window_mt.touch_pixel_position(win, i)
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return vec2(0, 0)
    else
        return vec2(touch.pixel_position_x, touch.pixel_position_y)
    end
end

function window_mt.touch_delta(win, i)
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return vec2(0, 0)
    else
        return vec2(touch.position_x - touch.prev_position_x, touch.position_y - touch.prev_position_y)
    end
end

function window_mt.touch_norm_delta(win, i)
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return vec2(0, 0)
    else
        return vec2(touch.norm_position_x - touch.prev_norm_position_x, touch.norm_position_y - touch.prev_norm_position_y)
    end
end

function window_mt.touch_pixel_delta(win, i)
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return vec2(0, 0)
    else
        return vec2(touch.pixel_position_x - touch.prev_pixel_position_x, touch.pixel_position_y - touch.prev_pixel_position_y)
    end
end

function window_mt.touches_began(win)
    local touches = {}
    for i, touch in ipairs(u._touches) do
        if touch.id and touch.began then
            table.insert(touches, i)
        end
    end
    return touches
end

function window_mt.touches_ended(win)
    local touches = {}
    for i, touch in ipairs(u._touches) do
        if touch.id and touch.ended then
            table.insert(touches, i)
        end
    end
    return touches
end

function window_mt.active_touches(win)
    local touches = {}
    for i, touch in ipairs(u._touches) do
        if touch.id and not touch.ended then
            table.insert(touches, i)
        end
    end
    return touches
end

function window_mt.touch_began(win, i)
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return false
    else
        return touch.began
    end
end

function window_mt.touch_ended(win, i)
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return false
    else
        return touch.ended
    end
end

local
function clear_touch(win)
    local u = win._event_data
    for i, touch in ipairs(u._touches) do
        if touch.ended then
            touch.id = false
        end
        touch.ended = false
        touch.began = false
        touch.prev_position_x = touch.position_x
        touch.prev_position_y = touch.position_y
        touch.prev_norm_position_x = touch.norm_position_x
        touch.prev_norm_position_y = touch.norm_position_y
        touch.prev_pixel_position_x = touch.pixel_position_x
        touch.prev_pixel_position_y = touch.pixel_position_y
    end
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

    u._key_state0 = {}
    u._key_presses = {}
    u._key_releases = {}

    -- mouse

    u._mouse_position_x = 0
    u._mouse_position_y = 0
    u._mouse_norm_position_x = 0
    u._mouse_norm_position_y = 0
    u._mouse_pixel_position_x = 0
    u._mouse_pixel_position_y = 0
    u._prev_mouse_position_x = 0
    u._prev_mouse_position_y = 0
    u._prev_mouse_norm_position_x = 0
    u._prev_mouse_norm_position_y = 0
    u._prev_mouse_pixel_position_x = 0
    u._prev_mouse_pixel_position_y = 0
    u._mouse_state0 = {}
    u._mouse_presses = {}
    u._mouse_releases = {}
    u._mouse_wheel_x = 0
    u._mouse_wheel_y = 0
    u._prev_mouse_wheel_x = 0
    u._prev_mouse_wheel_y = 0

    -- touch

    local max_touches = 10
    u._touches = {}

    for i = 1, max_touches do
        u._touches[i] = {
            id = false,
            began = false,
            ended = false,
            position_x = 0,
            position_y = 0,
            norm_position_x = 0,
            norm_position_y = 0,
            pixel_position_x = 0,
            pixel_position_y = 0,
            prev_position_x = 0,
            prev_position_y = 0,
            prev_norm_position_x = 0,
            prev_norm_position_y = 0,
            prev_pixel_position_x = 0,
            prev_pixel_position_y = 0,
        }
    end

    -- resize

    u._prev_height = 0
    u._prev_width = 0
    window.resized = resized

    if not am._main_window then
        am._main_window = window
    end
end
