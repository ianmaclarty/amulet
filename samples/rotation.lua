local am = amulet
local win = am.window{depth_buffer = true}

math.randomseed(os.time())

local vshader = [[
precision mediump float;
attribute vec3 pos;
attribute vec3 normal;
uniform mat4 MV;
uniform mat4 P;
varying vec3 v_color;
void main() {
    v_color = normal * 0.5 + 0.5;
    gl_Position = P * MV * vec4(pos, 1.0);
}
]]

local fshader = [[
precision mediump float;
varying vec3 v_color;
void main() {
    gl_FragColor = vec4(v_color, 1.0);
}
]]

local shader = am.program(vshader, fshader)

local
function load_model(name)
    local buf, stride, normals_offset, tex_offset = am.load_obj(name)
    local verts = buf:view("vec3", 0, stride)
    local normals = buf:view("vec3", normals_offset, stride)
    return am.draw_arrays("triangles")
        :bind("pos", verts)
        :bind("normal", normals)
end

local cube = load_model("cube.obj")

local rotating_cube = cube
    :rotate("MV", quat(0))
    :action(coroutine.create(function(node)
        while true do
            local angle = math.random() * 2 * math.pi
            local axis = math.normalize(vec3(math.random(), math.random(), math.random()) - 0.5)
            am.wait(am.tween{target = node, rotation = quat(angle, axis), time = 1,
                ease = am.ease.inout(am.ease.cubic)})
            am.wait(am.delay(0.5))
        end
    end))

local scene_group = am.group()

local translated_cube = rotating_cube:translate("MV", vec3(0, 0, -4))

local projection_matrix = math.perspective(math.rad(85), 1, 0.1, 10)

scene_group:append(translated_cube)

local scene = scene_group
    :bind("MV", mat4(1))
    :bind("P", projection_matrix)
    :bind_program(shader)
    :action(function()
        if win:key_pressed("escape") then
            win:close()
        end
    end)

win.root = scene
