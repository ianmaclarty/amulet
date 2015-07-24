local seq = 1

function amulet._update_action_seq()
    seq = seq + 1
end

local
function do_action(f, node)
    if type(f) == "thread" then
        if coroutine.status(f) == "dead" then
            return
        else
            local ok, res = coroutine.resume(f, node)
            if ok then
                return res
            else
                error(res)
            end
        end
    else
        return f(node)
    end
end

function amulet._execute_actions(actions, from, to)
    local t = amulet.frame_time
    for i = from, to do
        local action = actions[i]
        actions[i] = false
        if action.seq ~= seq and action.next_t <= t then
            action.seq = seq
            local res = do_action(action.func, action.node)
            if res then
                action.last_t = t
                if type(res) == "number" then
                    action.next_t = t + res
                else
                    action.next_t = t
                end
            else
                -- remove action
                local node_actions = action.node._actions
                local prev = nil
                for j = #node_actions, 1, -1 do
                    --log("query actions[%d] = %s", j, tostring(node_actions[j]))
                    node_actions[j], prev = prev, node_actions[j]
                    if prev == action then
                        goto next_action
                    end
                end
                assert(false) -- action not found
            end
        end
        ::next_action::
    end
end

local
function add_action(node, id, func)
    func = func or id
    local tp = type(func)
    if tp ~= "function" and tp ~= "thread" then
        error("action must be a function or coroutine (instead a "..tp..")", 2)
    end
    local action = {
        id = id,
        func = func,
        node = node,
        last_t = amulet.frame_time,
        next_t = amulet.frame_time,
        seq = seq, -- only execute on next frame
    }
    local actions = node._actions
    if not actions then
        actions = {}
        node._actions = actions
    end
    for i = #actions, 1, -1 do
        if actions[i].id == id then
            table.remove(actions, i)
            break
        end
    end
    table.insert(actions, action)
    --log("set actions[%d] = %s", n+1, tostring(action))
    return node -- for chaining
end

local
function cancel_action(node, id)
    local actions = node._actions
    if actions then
        for i = #actions, 1 do
            local action = actions[i]
            if action.id == id then
                table.remove(actions, i)
            end
        end
    end
end

_metatable_registry.scene_node.action = add_action
_metatable_registry.scene_node.cancel = cancel_action

amulet.actions = {}

function amulet.actions.delay(time)
    local done = false
    return function()
        if done then
            return
        else
            done = true
            return time
        end
    end
end

function amulet.actions.seq(seq)
    for i, val in ipairs(seq) do
        if type(val) ~= "function" then
            error("expecting a function at index "..i.." in action sequence", 2)
        end
    end
    local i = 1
    return function(node, dt)
        while true do
            local f = seq[i]
            if f then
                local r = f(node, dt)
                if r then
                    return r
                else
                    i = i + 1
                end
            else
                return
            end
        end
    end
end

function amulet.actions.loop(f)
    local curr = f()
    return function(node, dt)
        local r = curr(node, dt)
        if r then
            return r
        else
            curr = f()
            return curr(node, dt)
        end
    end
end
