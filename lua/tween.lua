function amulet.tween(tween_info)
    local time = 0
    local delay = 0
    local easing = amulet.easings.linear
    local action = nil
    local target = nil
    local final_values = {}
    for field, value in pairs(tween_info) do
        if field == "target" then
            target = value
        elseif field == "time" then
            time = value
        elseif field == "easing" then
            easing = value
        elseif field == "action" then
            action = value
        elseif field == "delay" then
            delay = value
        else
            final_values[field] = value
        end
    end
    if not target then
        error("no tween target given", 2)
    end
    local init_values = {}
    for field, value in pairs(final_values) do
        init_values[field] = target[field]
        if not init_values[field] then
            error("tween target has no field called '"..field.."'", 2)
        end
    end
    local t0 = am.frame_time
    return function()
        local elapsed = am.frame_time - t0
        if elapsed >= delay then
            elapsed = elapsed - delay
            if elapsed >= time then
                -- finished
                for field, value in ipairs(final_values) do
                    target[field] = value
                end
                if action then
                    action()
                end
                return
            end
            local t = elapsed / time
            for field, val0 in ipairs(init_vals) do
                local val = final_values[field]
                target[field] = val0 + easing(t) * (val - val0)
            end
        end
        return 0
    end
end

amulet.easings = {}

function amulet.easings.neg(f)
    return function(t)
        return 1 - f(1 - t)
    end
end

function amulet.easings.seq(f, g)
    return function(t)
        t = t * 2
        if t < 1 then
            return f(t)
        else
            return g(t - 1)
        end
    end
end

function amulet.easings.linear(t)
    return t
end

function amulet.easings.quadratic(t)
    return t * t
end

function amulet.easings.cubic(t)
    return t * t * t
end

function amulet.easings.hyperbolic(t)
    local s = 0.05
    return (1 / (1 + s - t) - 1) * s
end

function amulet.easings.sin(t)
    return (math.sin(math.pi * (t - 0.5)) + 1) * 0.5
end

function amulet.easings.overshoot(t)
    local s = 1.70158
    return t * t * ((s + 1) * t - s)
end

function amulet.easings.elastic(t)
    if t == 0 or t == 1 then
        return t
    end
    local p = 0.3
    local s = p / 4
    return math.pow(2, -10 * t) * math.sin((t - s) * (2 * math.pi) / p) + 1
end

function amulet.easings.bounce(t)
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
