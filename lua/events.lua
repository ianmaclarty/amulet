local window_mt = _metatable_registry.window

local empty_table = {} -- for active_touches, etc.

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
    local keys = nil
    local key_state0 = u._key_state0
    local key_releases = u._key_releases
    for key, presses in pairs(u._key_presses) do
        local state0 = key_state0[key]
        local releases = key_releases[key] or 0
        if not state0 and presses > 0 or state0 and releases <= 0 then
            if not keys then
                keys = {}
            end
            table.insert(keys, key)
        end
    end
    return keys or empty_table
end

function window_mt.key_pressed(win, key)
    key = tostring(key)
    local u = win._event_data
    return not u._key_state0[key] and (u._key_presses[key] or 0) > 0
end

function window_mt.keys_pressed(win)
    local u = win._event_data
    local keys = nil
    local key_state0 = u._key_state0
    for key, presses in pairs(u._key_presses) do
        if not key_state0[key] and presses > 0 then
            if not keys then
                keys = {}
            end
            table.insert(keys, key)
        end
    end
    return keys or empty_table
end

function window_mt.key_released(win, key)
    key = tostring(key)
    local u = win._event_data
    return u._key_state0[key] and (u._key_releases[key] or 0) > 0
end

function window_mt.keys_released(win)
    local u = win._event_data
    local keys = nil
    local key_state0 = u._key_state0
    local key_releases = u._key_releases
    for key, presses in pairs(u._key_presses) do
        if key_state0[key] and (key_releases[key] or 0) > 0 then
            if not keys then
                keys = {}
            end
            table.insert(keys, key)
        end
    end
    return keys or empty_table
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

function am._touch_begin(win, id, x, y, nx, ny, px, py, force)
    local u = win._event_data
    local free_i = nil
    for i, touch in ipairs(u._touches) do
        if not touch.id then
            free_i = i
            break
        end
    end
    if free_i then
        local touch = u._touches[free_i]
        touch.id = id
        touch.position_x = x
        touch.position_y = y
        touch.norm_position_x = nx
        touch.norm_position_y = ny
        touch.pixel_position_x = px
        touch.pixel_position_y = py
        touch.prev_position_x = touch.position_x
        touch.prev_position_y = touch.position_y
        touch.prev_norm_position_x = touch.norm_position_x
        touch.prev_norm_position_y = touch.norm_position_y
        touch.prev_pixel_position_x = touch.pixel_position_x
        touch.prev_pixel_position_y = touch.pixel_position_y
        touch.force = force
        touch.began = true
    end
end

function am._touch_end(win, id, x, y, nx, ny, px, py, force)
    local u = win._event_data
    for i, touch in ipairs(u._touches) do
        if touch.id == id then
            touch.position_x = x
            touch.position_y = y
            touch.norm_position_x = nx
            touch.norm_position_y = ny
            touch.pixel_position_x = px
            touch.pixel_position_y = py
            touch.force = force
            touch.ended = true
            return
        end
    end
end

function am._touch_move(win, id, x, y, nx, ny, px, py, force)
    local u = win._event_data
    for i, touch in ipairs(u._touches) do
        if touch.id == id and not touch.ended then
            touch.position_x = x
            touch.position_y = y
            touch.norm_position_x = nx
            touch.norm_position_y = ny
            touch.pixel_position_x = px
            touch.pixel_position_y = py
            touch.force = force
            return
        end
    end
end

function window_mt.touch_position(win, i)
    i = i or 1
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return vec2(0, 0)
    else
        return vec2(touch.position_x, touch.position_y)
    end
end

function window_mt.touch_norm_position(win, i)
    i = i or 1
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return vec2(0, 0)
    else
        return vec2(touch.norm_position_x, touch.norm_position_y)
    end
end

function window_mt.touch_pixel_position(win, i)
    i = i or 1
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return vec2(0, 0)
    else
        return vec2(touch.pixel_position_x, touch.pixel_position_y)
    end
end

function window_mt.touch_force(win, i)
    i = i or 1
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return 0
    else
        return touch.force
    end
end

function window_mt.touch_force_available(win)
    if am.platform == "ios" then
        return am.force_touch_available()
    else
        return false
    end
end

function window_mt.touch_delta(win, i)
    i = i or 1
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return vec2(0, 0)
    else
        return vec2(touch.position_x - touch.prev_position_x, touch.position_y - touch.prev_position_y)
    end
end

function window_mt.touch_norm_delta(win, i)
    i = i or 1
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return vec2(0, 0)
    else
        return vec2(touch.norm_position_x - touch.prev_norm_position_x, touch.norm_position_y - touch.prev_norm_position_y)
    end
end

function window_mt.touch_pixel_delta(win, i)
    i = i or 1
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return vec2(0, 0)
    else
        return vec2(touch.pixel_position_x - touch.prev_pixel_position_x, touch.pixel_position_y - touch.prev_pixel_position_y)
    end
end

function window_mt.touches_began(win)
    local u = win._event_data
    local touches
    for i, touch in ipairs(u._touches) do
        if touch.id and touch.began then
            if not touches then
                touches = {}
            end
            table.insert(touches, i)
        end
    end
    return touches or empty_table
end

function window_mt.touches_ended(win)
    local u = win._event_data
    local touches
    for i, touch in ipairs(u._touches) do
        if touch.id and touch.ended then
            if not touches then
                touches = {}
            end
            table.insert(touches, i)
        end
    end
    return touches or empty_table
end

function window_mt.active_touches(win)
    local u = win._event_data
    local touches
    for i, touch in ipairs(u._touches) do
        if touch.id and not touch.ended then
            if not touches then
                touches = {}
            end
            table.insert(touches, i)
        end
    end
    return touches or empty_table
end

function window_mt.touch_active(win, i)
    i = i or 1
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return false
    else
        return touch.id and not touch.ended
    end
end

function window_mt.touch_began(win, i)
    i = i or 1
    local u = win._event_data
    local touch = u._touches[i]
    if not touch then
        return false
    else
        return touch.began
    end
end

function window_mt.touch_ended(win, i)
    i = i or 1
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

-- controllers

local max_controllers = 8
local controllers = {}
for i = 1, max_controllers do
    controllers[i] = {id = false}
end

function am._controller_attached(index, id)
    --log("controller attached %d %d", index, id)
    if index < max_controllers then
        controllers[index + 1] = {
            id = id,
            just_attached = true,
            left_x = 0,
            left_y = 0,
            right_x = 0,
            right_y = 0,
            lt = 0,
            rt = 0,
            button_state0 = {},
            button_presses = {},
            button_releases = {},
        }
    end
end

function am._controller_detached(id)
    --log("controller detached "..id)
    for i, controller in ipairs(controllers) do
        if controller.id == id then
            controller.just_detached = true
            controller.id = false
            return
        end
    end
end

function am._controller_button_down(id, button)
    --log("controller button down "..button.." "..id)
    for i, controller in ipairs(controllers) do
        if controller.id == id then
            controller.button_presses[button] = (controller.button_presses[button] or 0) + 1
            return
        end
    end
end

function am._controller_button_up(id, button)
    --log("controller button up "..button.." "..id)
    for i, controller in ipairs(controllers) do
        if controller.id == id then
            controller.button_releases[button] = (controller.button_releases[button] or 0) + 1
            return
        end
    end
end

function am._controller_axis_motion(id, axis, val)
    --log("controller axis motion "..axis.." "..val.." "..id)
    for i, controller in ipairs(controllers) do
        if controller.id == id then
            controller[axis] = val
            return
        end
    end
end

local
function clear_controllers()
    for i, controller in ipairs(controllers) do
        if controller.id then
            for button, presses in pairs(controller.button_presses) do
                local state0 = controller.button_state0[button]
                local releases = controller.button_releases[button] or 0
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
                controller.button_state0[button] = new_state
                controller.button_presses[button] = presses
                controller.button_releases[button] = releases
            end
        end
        controller.just_attached = nil
        controller.just_detached = nil
    end
end

am._register_post_frame_func(clear_controllers)

function am.controller_lt_val(index)
    return controllers[index].lt or 0
end

function am.controller_rt_val(index)
    return controllers[index].rt or 0
end

function am.controller_lstick_pos(index)
    local controller = controllers[index]
    if not controller or not controller.id then
        return vec2(0)
    end
    return vec2(controller.left_x, controller.left_y)
end

function am.controller_rstick_pos(index)
    local controller = controllers[index]
    if not controller or not controller.id then
        return vec2(0)
    end
    return vec2(controller.right_x, controller.right_y)
end

function am.controller_button_pressed(index, button)
    local controller = controllers[index]
    if not controller or not controller.id then
        return false
    end
    return not controller.button_state0[button] and (controller.button_presses[button] or 0) > 0
end

function am.controller_button_released(index, button)
    local controller = controllers[index]
    if not controller or not controller.id then
        return false
    end
    return controller.button_state0[button] and (controller.button_releases[button] or 0) > 0
end

function am.controller_button_down(index, button)
    local controller = controllers[index]
    if not controller or not controller.id then
        return false
    end
    local state0 = controller.button_state0[button]
    local presses = controller.button_presses[button] or 0
    local releases = controller.button_releases[button] or 0
    return not state0 and presses > 0 or state0 and releases <= 0
end

function am.controller_present(index)
    local controller = controllers[index]
    if controller and controller.id then
        return true
    else
        return false
    end
end

function am.controller_attached(index)
    local controller = controllers[index]
    if controller then
        return controller.just_attached
    else
        return false
    end
end

function am.controller_detached(index)
    local controller = controllers[index]
    if controller then
        return controller.just_detached
    else
        return false
    end
end

function am.controllers_present()
    local res = nil
    for i, controller in ipairs(controllers) do
        if controller.id then
            if not res then
                res = {}
            end
            table.insert(res, i)
        end
    end
    return res or empty_table
end

function am.controllers_attached()
    local res = nil
    for i, controller in ipairs(controllers) do
        if controller.just_attached then
            if not res then
                res = {}
            end
            table.insert(res, i)
        end
    end
    return res or empty_table
end

function am.controllers_detached()
    local res = nil
    for i, controller in ipairs(controllers) do
        if controller.just_detached then
            if not res then
                res = {}
            end
            table.insert(res, i)
        end
    end
    return res or empty_table
end

function am.rumble(index, duration, strengh) 
    if am._rumble then
        local controller = controllers[index]
        if not controller or not controller.id then
            return
        end
        am._rumble(controller.id, duration, strengh)
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

function am._reset_window_event_data(window)
    local u = window._event_data
    table.clear(u._key_state0)
    table.clear(u._key_presses)
    table.clear(u._key_releases)
    table.clear(u._mouse_state0)
    table.clear(u._mouse_presses)
    table.clear(u._mouse_releases)
    for i, touch in ipairs(u._touches) do
        touch.id = false
        touch.ended = false
        touch.began = false
    end
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
            force = 0,
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

-- app focus
function am._became_active()
    am._did_just_become_active = true
end

am._register_pre_frame_func(function()
    if am._did_just_become_active then
        am._in_become_active_frame = true
        am._did_just_become_active = false
    else
        am._in_become_active_frame = false
    end
end)

function am.app_became_active()
    return am._in_become_active_frame
end
