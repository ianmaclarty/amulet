local seq = 1

function amulet._update_action_seq()
    seq = seq + 1
end

function amulet._execute_actions(actions, from, to)
    local t = amulet.frame_time
    for i = from, to do
        local action = actions[i]
        actions[i] = false
        if action.seq ~= seq and action.next_t <= t then
            action.seq = seq
            local res = action.func(action.node, t - action.last_t)
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
function add_action(node, func, delay)
    delay = delay or 0
    local action = {
        func = func,
        node = node,
        last_t = amulet.frame_time,
        next_t = amulet.frame_time + delay,
        seq = seq, -- only execute on next frame
    }
    local actions = node._actions
    if not actions then
        actions = {}
        node._actions = actions
    end
    local n = #actions
    actions[n + 1] = action
    --log("set actions[%d] = %s", n+1, tostring(action))
    return node -- for chaining
end

_metatable_registry.scene_node.action = add_action
