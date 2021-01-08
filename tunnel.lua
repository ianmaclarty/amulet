local win = am.window{
    title = "tunnel",
}

-- constants
local num_edges = 7
local num_rings = 50
local ring_gap = 2
local forward = vec3(0, 0, -1)
local up = vec3(0, 1, 0)

-- state
local player_pos = vec3(0)
local player_speed = vec3(0, 0, -50)
local ring_color = vec4(1)
local radius = 10

-- tunnel shader
local t_vert_shader = [[
    precision highp float;
    attribute vec3 vert;
    attribute vec4 color;
    uniform mat4 MV;
    uniform mat4 P;
    varying vec4 v_color;
    void main() {
        v_color = color;
        gl_Position = P * MV * vec4(vert, 1.0);
    }
]]

local t_frag_shader = [[
    precision mediump float;
    varying vec4 v_color;
    void main() {
        gl_FragColor = v_color;
    }
]]

-- postprocess shader
local pp_vert_shader = [[
    precision highp float;
    attribute vec2 vert;
    attribute vec2 uv;
    varying vec2 v_uv;
    uniform mat4 MV;
    uniform mat4 P;
    void main() {
        v_uv = uv;
        gl_Position = P * MV * vec4(vert, 0.0, 1.0);
    }
]]
local pp_frag_shader = [[
    precision mediump float;
    uniform sampler2D tex;
    varying vec2 v_uv;
    void main() {
        vec2 coord = gl_FragCoord.xy;
        float noise1 = fract(sin(dot(coord, vec2(12.9898,78.233))) * 43.5453);
        gl_FragColor = fract(texture2D(tex, v_uv + vec2(noise1 * 0.01, 0.0)));
    }
]]

-- create tunnel vertices

local verts = am.buffer(num_edges * num_rings * 12):view"vec3"
local colors = am.buffer(num_edges * num_rings * 16):view"vec4"

function add_ring(i, pos)
    local v = (i - 1) * num_edges + 1
    local r = math.random()
    for j = 1, num_edges do
        local angle = (j - 1) / num_edges * 2 * math.pi + r
        local x = math.cos(angle) * radius
        local y = math.sin(angle) * radius
        verts[v] = vec3(x, y, 0) + pos
        if math.random() > 0.1 then
            colors[v] = ring_color
        else
            colors[v] = vec4(math.randvec3(), 1)
        end
        local delta_color = am.delta_time * (math.randvec3() - 0.5) * 3
        ring_color = math.clamp(ring_color + vec4(delta_color, 1), vec4(0), vec4(1))
        v = v + 1
    end
    radius = (math.sin(am.frame_time / 10) + 1) * 0.5 * 60
end

for i = 1, num_rings do
    add_ring(i, vec3(0, 0, -i * ring_gap + 10))
end


local lines = {}
local l = 1
local v = 1
for i = 1, num_rings do
    for j = 1, num_edges do
        if j > 1 then
            lines[l] = v - 1
            lines[l + 1] = v
        else
            lines[l] = v + num_edges - 1
            lines[l + 1] = v
        end
        v = v + 1
        l = l + 2
    end
end

-- create tunnel node
local tunnel_node =
    am.use_program(am.program(t_vert_shader, t_frag_shader))
    ^ am.bind{
        vert = verts,
        color = colors,
    }
    ^ am.draw("lines", am.uint_elem_array(lines))


-- create main scene node
local main_scene =
    am.postprocess{
        auto_clear = false,
        width = 320,
        height = 240,
        program = am.program(pp_vert_shader, pp_frag_shader)
    }
    ^ am.bind{P = math.perspective(math.rad(70), win.width/win.height, 1, 1000)}
    ^ am.lookat(player_pos, player_pos + forward, up):tag"camera"
    ^ tunnel_node

-- main action (run once per frame)
local next_ring = 1
local t = 0
function main_action()
    forward = math.normalize(vec3(-win:mouse_norm_position(), -1))
    local cam = main_scene"camera"
    cam.eye = player_pos
    cam.center = player_pos + forward
    player_pos = player_pos + player_speed * am.delta_time
    
    t = t + am.delta_time
    while -player_speed.z / t >= ring_gap do
        local ring_pos = player_pos - vec3(0, 0, num_rings * ring_gap - 10)
        add_ring(next_ring, ring_pos)
        t = t + ring_gap / player_speed.z
        next_ring = next_ring % num_rings + 1
    end
end

-- attach main_action to main_scene
main_scene:action(main_action)

-- attach main scene to window
win.scene = main_scene
