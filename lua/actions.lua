local seq = 1

function amulet._update_action_seq()
    seq = seq + 1
end

local
function do_action(f, node)
    if type(f) == "thread" then
        if coroutine.status(f) == "dead" then
            return true
        else
            local ok, res = coroutine.resume(f, node)
            if ok then
                return
            else
                res = amulet._traceback(res, f)
                error("__coroutine__"..res)
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
        if action.seq ~= seq then
            action.seq = seq
            if do_action(action.func, action.node) then
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

function amulet.delay(time)
    local t = 0
    return function()
        t = t + amulet.delta_time
        if t >= time then
            return true
        end
    end
end

function amulet.series(funcs)
    for i, val in ipairs(funcs) do
        if type(val) ~= "function" and type(val) ~= "thread" then
            error("expecting a function or coroutine at index "..i, 2)
        end
    end
    local i = 1
    return function(node)
        while true do
            local f = funcs[i]
            if f then
                if do_action(f, node) then
                    i = i + 1
                else
                    return
                end
            else
                return true
            end
        end
    end
end

function amulet.parallel(funcs)
    for i, val in ipairs(funcs) do
        if type(val) ~= "function" and type(val) ~= "thread" then
            error("expecting a function or coroutine at index "..i, 2)
        end
    end
    funcs = table.shallow_copy(funcs)
    return function(node)
        for i = #funcs, 1, -1 do
            if do_action(funcs[i], node) then
                table.remove(funcs, i)
            end
        end
        if #funcs == 0 then
            return true
        end
    end
end

function amulet.wait(func, node)
    local _, main = coroutine.running()
    if main then
        error("wait may only be called from within a coroutine", 2)
    end
    repeat 
        coroutine.yield()
    until do_action(func, node)
    return true
end
