local mix = math.mix

function am.tween(target, time, final_values, ease)
    if type(target) == "number" then
        target, time, final_values, ease = nil, target, time, final_values
    end
    if not time then
        error("missing tween time")
    end
    if not final_values then
        error("missing final tween values")
    end
    if type(final_values) ~= "table" then
        error("missing tween final values table")
    end
    ease = ease or am.ease.linear
    local vecs = {}
    local comps = {}
    for k, v in pairs(final_values) do
        if type(k) ~= "string" then
            error("tween field must be a string", 2)
        end
        local vec, comp = k:match("^(.+)%.(.+)$")
        if vec then
            vecs[k] = vec
            comps[k] = comp
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
                if vecs[field] then
                    local v, c = vecs[field], comps[field]
                    init_values[field] = target[v][c]
                else
                    init_values[field] = target[field]
                end
            end
        end
        elapsed = elapsed + am.delta_time
        if elapsed >= time then
            -- done
            for field, value in pairs(final_values) do
                if vecs[field] then
                    local v, c = vecs[field], comps[field]
                    target[v] = target[v](c, value)
                else
                    target[field] = value
                end
            end
            return true
        end
        local t = elapsed / time
        for field, val0 in pairs(init_values) do
            local val = mix(val0, final_values[field], ease(t))
            if vecs[field] then
                local v, c = vecs[field], comps[field]
                target[v] = target[v](c, val)
            else
                target[field] = val
            end
        end
    end
end

am.ease = {}

function am.ease.out(f)
    return function(t)
        return 1 - f(1 - t)
    end
end

function am.ease.inout(f, g)
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

function am.ease.linear(t)
    return t
end

function am.ease.quadratic(t)
    return t * t
end

function am.ease.cubic(t)
    return t * t * t
end

function am.ease.hyperbola(t)
    local s = 0.05
    return (1 / (1 + s - t) - 1) * s
end

function am.ease.sine(t)
    return (math.sin(math.pi * (t - 0.5)) + 1) * 0.5
end

function am.ease.windup(t)
    local s = 1.70158
    return t * t * ((s + 1) * t - s)
end

function am.ease.elastic(t)
    if t == 0 or t == 1 then
        return t
    end
    local p = 0.3
    local s = p / 4
    return math.pow(2, -10 * t) * math.sin((t - s) * (2 * math.pi) / p) + 1
end

function am.ease.bounce(t)
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

function am.ease.cubic_bezier(x1, y1, x2, y2)
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
