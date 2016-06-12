-- rects:
-- 1-4
-- |\|
-- 2-3

local cached_rect_indices = am.ushort_elem_array{1, 2, 3, 1, 3, 4}

function am.rect_indices() 
    return cached_rect_indices
end

function am.quad_indices(n)
    local indices = {}
    local j = 0
    local m = (n - 1) * 6
    for i = 0, m, 6 do
        indices[i + 1] = j + 1
        indices[i + 2] = j + 2
        indices[i + 3] = j + 3
        indices[i + 4] = j + 1
        indices[i + 5] = j + 3
        indices[i + 6] = j + 4
        j = j + 4
    end
    if n * 4 < 2^16 then
        return am.ushort_elem_array(indices)
    else
        return am.uint_elem_array(indices)
    end
end

local view_type_size = {
    float = 4,
    vec2 = 8,
    vec3 = 12,
    vec4 = 16,
}

function am.quad_mesh(max, attrs)
    local stride = 0
    for attr, view_type in pairs(attrs) do
        stride = stride + view_type_size[view_type]
    end
    local buffer = am.buffer(max * stride * 4)
    local bindings = {}
    local offset = 0
    for attr, view_type in pairs(attrs) do
        local view = buffer:view(view_type, offset, stride)
        local sz = view_type_size[view_type]
        bindings[attr] = view
        offset = offset + sz
    end
    local node = am.bind(bindings) ^ am.draw("triangles", am.quad_indices(max))
    for attr, view in pairs(bindings) do
        node["quad_"..attr] = function(node, q, vals)
            view:set(vals, (q - 1) * 4 + 1, 4)
        end
    end
    return node
end

function am.rect_verts_2d(x1, y1, x2, y2)
    return am.vec2_array{x1, y2, x1, y1, x2, y1, x2, y2}
end

function am.rect_verts_3d(x1, y1, x2, y2)
    return am.vec3_array{x1, y2, 0, x1, y1, 0, x2, y1, 0, x2, y2, 0}
end

local offsets = {
    x1 = {1, 2, 1},
    y1 = {2, 3, 2},
    x2 = {3, 4, 1},
    y2 = {1, 4, 2},
}

local rect_props = {}
for field, os in pairs(offsets) do
    local os1, os2, c = unpack(os)
    rect_props["get_"..field] = function(node)
        return node.verts[os1][c]
    end
    rect_props["set_"..field] = function(node, val)
        local verts = node.verts
        verts[os1] = verts[os1](c, val)
        verts[os2] = verts[os2](c, val)
    end
end
function rect_props:get_color()
    return self"bind".color
end
function rect_props:set_color(color)
    self"bind".color = color
end

function am.rect(x1, y1, x2, y2, color)
    color = color or vec4(1)
    local verts = am.rect_verts_2d(x1, y1, x2, y2)
    local node = am.use_program(am.shaders.color2d)
        ^am.blend"alpha"
        ^am.bind{
            vert = verts,
            color = color,
        }
        ^am.draw("triangles", cached_rect_indices)
    node.verts = verts
    for prop, func in pairs(rect_props) do
        node[prop] = func
    end
    node:tag("rect")
    return node
end

-- circle:

local default_circle_sides = 255

function am.circle_indices(sides)
    sides = sides or default_circle_sides
    local t = {}
    local i = 1
    for s = 2, sides + 1 do
        t[i] = 1
        t[i + 1] = s
        t[i + 2] = (s - 1) % sides + 2
        i = i + 3
    end
    if sides < 2^15 then
        return am.ushort_elem_array(t)
    else
        return am.uint_elem_array(t)
    end
end

local cached_unit_circle_verts_2d = {}

function am.unit_circle_verts_2d(sides)
    sides = sides or default_circle_sides
    if cached_unit_circle_verts_2d[sides] then
        return cached_unit_circle_verts_2d[sides]
    end
    local t = {0, 0}
    local angle = 0
    local dangle = 2 * math.pi / sides
    local cos, sin = math.cos, math.sin
    local i = 3
    for s = 0, sides - 1 do
        t[i] = cos(angle)
        t[i + 1] = sin(angle)
        i = i + 2
        angle = angle + dangle
    end
    local verts = am.vec2_array(t)
    cached_unit_circle_verts_2d[sides] = verts
    return verts
end

function am.circle(center, radius, color, sides)
    sides = sides or default_circle_sides
    color = color or vec4(1)
    local verts = am.unit_circle_verts_2d(sides)
    local node = 
        am.translate(center)
        ^am.scale(radius)
        ^am.use_program(am.shaders.color2d)
        ^am.blend"alpha"
        ^am.bind{
            vert = verts,
            color = color,
        }
        ^am.draw("triangles", am.circle_indices(sides))
    node.verts = verts
    function node:get_center()
        return center
    end
    function node:set_center(c)
        center = c
        node"translate".position2d = c
    end
    function node:get_radius()
        return radius
    end
    function node:set_radius(r)
        radius = r
        node"scale".scale2d = vec2(radius)
    end
    function node:get_color()
        return color
    end
    function node:set_color(c)
        color = c
        node"bind".color = color
    end
    node:tag"circle"
    return node
end

function am.line(point1, point2, thickness, color)
    thickness = thickness or 1
    color = color or vec4(1)
    local verts = am.buffer(4 * 2 * 4):view("vec2")
    local
    function set_verts()
        local slope = math.normalize(point2 - point1)
        local os1_x = -slope.y * thickness / 2
        local os1_y = slope.x * thickness / 2
        local os2_x = slope.y * thickness / 2
        local os2_y = -slope.x * thickness / 2
        local x1 = point1.x
        local y1 = point1.y
        local x2 = point2.x
        local y2 = point2.y
        verts:set{os1_x + x1, os1_y + y1, os2_x + x1, os2_y + y1, os2_x + x2, os2_y + y2, os1_x + x2, os1_y + y2}
    end
    set_verts()
    local node =
        am.use_program(am.shaders.color2d)
        ^am.blend"alpha"
        ^am.bind{
            vert = verts,
            color = color,
        }
        ^am.draw("triangles", cached_rect_indices)
    function node:get_point1()
        return point1
    end
    function node:set_point1(p)
        point1 = p
        set_verts()
    end
    function node:get_point2()
        return point2
    end
    function node:set_point2(p)
        point2 = p
        set_verts()
    end
    function node:get_thickness()
        return thickness
    end
    function node:set_thickness(t)
        thickness = t
        set_verts()
    end
    function node:get_color()
        return color
    end
    function node:set_color(c)
        color = c
        node"bind".color = color
    end
    node:tag"line"
    return node
end
