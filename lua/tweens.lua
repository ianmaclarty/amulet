function amulet.tween(tween_info)
    local time = 0
    local easing = amulet.easing.linear
    local target = nil
    local final_values = {}
    for field, value in pairs(tween_info) do
        if field == "target" then
            target = value
        elseif field == "time" then
            time = value
        elseif field == "easing" then
            easing = value
        else
            final_values[field] = value
        end
    end
    local init_values
    local elapsed = 0
    return function(node)
        if not init_values then
            if not target then
                target = node
            elseif type(target) == "string" then
                target = node[target]
            end
            init_values = {}
            for field, value in pairs(final_values) do
                init_values[field] = target[field]
            end
        end
        elapsed = elapsed + amulet.delta_time
        if elapsed >= time then
            -- done
            for field, value in pairs(final_values) do
                target[field] = value
            end
            return
        end
        local t = elapsed / time
        for field, val0 in pairs(init_values) do
            local val = final_values[field]
            target[field] = val0 + easing(t) * (val - val0)
        end
        return 0
    end
end

amulet.easing = {}

function amulet.easing.reverse(f)
    return function(t)
        return 1 - f(1 - t)
    end
end

function amulet.easing.onetwo(f, g)
    return function(t)
        t = t * 2
        if t < 1 then
            return f(t) * 0.5
        else
            return g(t - 1) * 0.5 + 0.5
        end
    end
end

function amulet.easing.linear(t)
    return t
end

function amulet.easing.quadratic(t)
    return t * t
end

function amulet.easing.cubic(t)
    return t * t * t
end

function amulet.easing.hyperbola(t)
    local s = 0.05
    return (1 / (1 + s - t) - 1) * s
end

function amulet.easing.sine(t)
    return (math.sin(math.pi * (t - 0.5)) + 1) * 0.5
end

function amulet.easing.windup(t)
    local s = 1.70158
    return t * t * ((s + 1) * t - s)
end

function amulet.easing.elastic(t)
    if t == 0 or t == 1 then
        return t
    end
    local p = 0.3
    local s = p / 4
    return math.pow(2, -10 * t) * math.sin((t - s) * (2 * math.pi) / p) + 1
end

function amulet.easing.bounce(t)
    local s = 7.5625
    local p = 2.75
    local l
    if t < 1 / p then
        l = s * t * t
    else
        if t < 2 / p then
            t = t - 1.5 / p
            l = s * t * t + 0.75
        else
            if t < 2.5 / p then
                t = t - 2.25 / p
                l = s * t * t + 0.9375
            else
                t = t - 2.625 / p
                l = s * t * t + 0.984375
            end
        end
    end
    return l
end
