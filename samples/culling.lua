local am = amulet
local win = am.window{depth_buffer = true}

local vshader = [[
precision mediump float;
attribute vec3 pos;
attribute vec3 normal;
uniform mat4 M;
uniform mat4 P;
varying vec3 v_color;
void main() {
    v_color = normal * 0.5 + 0.5;
    gl_Position = P * M * vec4(pos, 1.0);
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
        :bind_array("pos", verts)
        :bind_array("normal", normals)
end

local cube = load_model("cube.obj")

local modelview_matrix = mat4(1)

local rotating_cube = cube
    :read_mat4("M")
    :action(function(node)
        modelview_matrix = node.value
        --log(modelview_matrix)
    end)
local scene_group = am.group()

local translated_cube = rotating_cube:translate("M", 0, 0, -7)

local projection_matrix = math.perspective(math.rad(85), 1, 0.1, 10)

scene_group:append(translated_cube)

local scene = scene_group
    :bind_mat4("M", mat4(1))
    :bind_mat4("P", projection_matrix)
    :bind_program(shader)
    :action(function()
        translated_cube.xy = win:mouse_position().xy * 10
        log(math.sphere_visible(projection_matrix * modelview_matrix, vec3(0), math.sqrt(3)))
        if win:key_pressed("escape") then
            win:close()
        end
    end)

win.root = scene
