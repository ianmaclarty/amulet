local seq = 1

am._register_pre_frame_func(function()
    seq = seq + 1
end)

function am.step_action(f, node)
    if type(f) == "thread" then
        if coroutine.status(f) == "dead" then
            return true
        else
            local ok, res = coroutine.resume(f, node)
            if ok then
                return
            else
                res = am._traceback(res, f)
                error("__coroutine__"..res)
            end
        end
    else
        return f(node)
    end
end

local do_action = am.step_action

function am._execute_actions(actions, from, to)
    local t = am.frame_time
    local priority = 1
    repeat
        local next_priority = 10000000
        for i = from, to do
            local action = actions[i]
            if action then
                local p = action.priority
                if p <= priority then
                    actions[i] = false
                    if action.seq ~= seq then
                        action.seq = seq
                        if do_action(action.func, action.node) then
                            -- remove action
                            local node_actions = action.node._actions
                            for j = 1, #node_actions do
                                --log("query actions[%d] = %s", j, tostring(node_actions[j]))
                                if node_actions[j] == action then
                                    table.remove(node_actions, j)
                                    break
                                end
                            end
                        end
                    end
                elseif p <= next_priority then
                    next_priority = p
                end
            end
        end
        priority = next_priority
    until priority == 10000000
end

local
function add_action(node, id, func, priority)
    local tp = type(func)
    if tp == "number" or tp == "nil" then
        func, priority = id, func
    end
    tp = type(func)
    if tp ~= "function" and tp ~= "thread" then
        error("action must be a function or coroutine (instead a "..tp..")", 2)
    end
    local action = {
        id = id,
        func = func,
        node = node,
        seq = seq, -- only execute on next frame
        priority = priority or 1
    }
    local actions = node._actions
    if not actions then
        actions = {}
        node._actions = actions
    end
    for i = #actions, 1, -1 do
        local act = actions[i]
        if act.id == id then
            act.seq = seq -- make sure action does not execute this frame if it hasn't already
            table.remove(actions, i)
            break
        end
    end
    table.insert(actions, action)
    --log("set actions[%d] = %s", n+1, tostring(action))
    return node -- for chaining
end

local
function add_late_action(node, id, func)
    if not func then
        func = id
    end
    add_action(node, id, func, 100)
end

local
function cancel_action(node, id)
    local actions = node._actions
    if actions then
        for i = #actions, 1, -1 do
            local action = actions[i]
            if action.id == id then
                action.seq = seq -- make sure action does not execute this frame if it hasn't already
                table.remove(actions, i)
            end
        end
    end
end

local
function update(node)
    am._execute_actions(am._node_add_actions(node))
end

_metatable_registry.scene_node.action = add_action
_metatable_registry.scene_node.late_action = add_late_action
_metatable_registry.scene_node.cancel = cancel_action
_metatable_registry.scene_node.update = update

am.actions = {}

function am.delay(time)
    local t = 0
    return function()
        t = t + am.delta_time
        if t >= time then
            return true
        end
    end
end

function am.series(funcs)
    if type(funcs) ~= "table" then
        error("expecting a table of actions as the single argument to series", 2)
    end
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

function am.loop(func)
    local action
    return function(node)
        if not action then 
            action = func(node)
        end
        if do_action(action, node) then
            action = nil
        end
    end
end

function am.parallel(funcs)
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

function am.wait(func, node)
    local thread = coroutine.running()
    if not thread then
        error("wait may only be called from within a coroutine", 2)
    end
    repeat 
        coroutine.yield()
    until do_action(func, node)
    return true
end
