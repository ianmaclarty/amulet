-- rects:
-- 1-4
-- |\|
-- 2-3

function am.rect_indices() 
    return am.ushort_elem_array{1, 2, 3, 1, 3, 4}
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
    local verts = am.rect_verts_3d(x1, y1, x2, y2)
    local node = am.use_program(am.shaders.color)
        ^am.bind{
            vert = verts,
            color = color,
        }
        ^am.draw("triangles", am.rect_indices())
    node.verts = verts
    for prop, func in pairs(rect_props) do
        node[prop] = func
    end
    node:tag("rect")
    return node
end

-- ellipse:

local default_ellipse_sides = 255

function am.ellipse_indices(sides)
    sides = sides or default_ellipse_sides
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

function am.ellipse_verts_2d(center, xrad, yrad, sides)
    sides = sides or default_ellipse_sides
    local x = center.x
    local y = center.y
    local t = {x, y}
    local angle = 0
    local dangle = 2 * math.pi / sides
    local cos, sin = math.cos, math.sin
    local i = 3
    for s = 0, sides - 1 do
        t[i] = cos(angle) * xrad + x
        t[i + 1] = sin(angle) * yrad + y
        i = i + 2
        angle = angle + dangle
    end
    return am.vec2_array(t)
end

function am.ellipse_verts_3d(center, xrad, yrad, sides)
    sides = sides or default_ellipse_sides
    local x = center.x
    local y = center.y
    local t = {x, y, 0}
    local angle = 0
    local dangle = 2 * math.pi / sides
    local cos, sin = math.cos, math.sin
    local i = 4
    for s = 0, sides - 1 do
        t[i] = cos(angle) * xrad + x
        t[i + 1] = sin(angle) * yrad + y
        t[i + 2] = 0
        i = i + 3
        angle = angle + dangle
    end
    return am.vec3_array(t)
end

function am.ellipse(center, xrad, yrad, color, sides)
    sides = sides or default_ellipse_sides
    color = color or vec4(1)
    local verts = am.ellipse_verts_3d(center, xrad, yrad, sides)
    local node = am.use_program(am.shaders.color)
        ^am.bind{
            vert = verts,
            color = color,
        }
        ^am.draw("triangles", am.ellipse_indices(sides))
    node.verts = verts
    function node:get_color()
        return color
    end
    function node:set_color(c)
        color = c
        node"bind".color = color
    end
    node:tag("ellipse")
    return node
end

function am.circle(center, rad, color, sides)
    return am.ellipse(center, rad, rad, color, sides):tag("circle")
end
