local mix = math.mix

function amulet.tween(tween_info)
    local time = 0
    local ease = amulet.ease.linear
    local target = nil
    local final_values = {}
    for field, value in pairs(tween_info) do
        if field == "target" then
            target = value
        elseif field == "time" then
            time = value
        elseif field == "ease" then
            ease = value
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
            return true
        end
        local t = elapsed / time
        for field, val0 in pairs(init_values) do
            local val = final_values[field]
            target[field] = mix(val0, val, ease(t))
        end
    end
end

amulet.ease = {}

function amulet.ease.out(f)
    return function(t)
        return 1 - f(1 - t)
    end
end

function amulet.ease.inout(f, g)
    g = g or f
    return function(t)
        t = t * 2
        if t < 1 then
            return f(t) * 0.5
        else
            return 1 - 0.5 * g(2 - t)
        end
    end
end

function amulet.ease.linear(t)
    return t
end

function amulet.ease.quadratic(t)
    return t * t
end

function amulet.ease.cubic(t)
    return t * t * t
end

function amulet.ease.hyperbola(t)
    local s = 0.05
    return (1 / (1 + s - t) - 1) * s
end

function amulet.ease.sine(t)
    return (math.sin(math.pi * (t - 0.5)) + 1) * 0.5
end

function amulet.ease.windup(t)
    local s = 1.70158
    return t * t * ((s + 1) * t - s)
end

function amulet.ease.elastic(t)
    if t == 0 or t == 1 then
        return t
    end
    local p = 0.3
    local s = p / 4
    return math.pow(2, -10 * t) * math.sin((t - s) * (2 * math.pi) / p) + 1
end

function amulet.ease.bounce(t)
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

function amulet.ease.cubic_bezier(x1, y1, x2, y2)
    local cx = 3 * x1
    local bx = 3 * (x2 - x1) - cx
    local ax = 1 - cx - bx
    local cy = 3 * y1
    local by = 3 * (y2 - y1) - cy
    local ay = 1 - cy - by
    local function sampleCurveX(t)
        return ((ax * t + bx) * t + cx) * t
    end
    local function solveCurveX(x, epsilon)
        local t0, t1, t2, x2, d2, i
        t2 = x
        for i = 0, 7 do
            x2 = sampleCurveX(t2) - x
            if math.abs(x2) < epsilon then
                return t2
            end
            d2 = (3 * ax * t2 + 2 * bx) * t2 + cx
            if math.abs(d2) < 1e-6 then
                break
            end
            t2 = t2 - x2 / d2
        end
        t0 = 0
        t1 = 1
        t2 = x
        if t2 < t0 then
            return t0
        end
        if t2 > t1 then
            return t1
        end
        while t0 < t1 do
            x2 = sampleCurveX(t2)
            if math.abs(x2 - x) < epsilon then
                return t2
            end
            if x > x2 then
                t0 = t2
            else
                t1 = t2
            end
            t2 = (t1 - t0) / 2 + t0
        end
        return t2
    end
    return function(t)
        local t2 = solveCurveX(t, 0.001)
        return ((ay * t2 + by) * t2 + cy) * t2
    end
end
