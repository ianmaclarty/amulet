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
    return 
        am.bind{
            pos = verts,
            normal = normals,
        }
        ^am.draw("triangles")
end

local cube = load_model("cube.obj")

local modelview_matrix = mat4(1)

local rotating_cube = 
    am.read_uniform("M")
    :action(function(node)
        modelview_matrix = node.value
        --log(modelview_matrix)
    end)
    ^cube
local scene_group = am.group()

local translated_cube = am.translate("M", vec3(0, 0, -7)) ^ rotating_cube

local projection_matrix = math.perspective(math.rad(85), 1, 0.1, 10)

scene_group:append(translated_cube)

local bg = am.rect(win.left, win.bottom, win.right, win.top, vec4(0, 0, 0, 1))

local scene = 
    am.use_program(shader)
        :action(function()
            translated_cube.position = vec3(win:mouse_norm_position().xy * 10, translated_cube.position.z)
            if math.sphere_visible(projection_matrix * modelview_matrix, vec3(0), math.sqrt(3)) then
                bg.color = vec4(0, 0, 0, 1)
            else
                bg.color = vec4(1, 0, 0, 1)
            end
            if win:key_pressed("escape") then
                win:close()
            end
        end)
    ^am.bind{
        M = mat4(1),
        P = projection_matrix,
    }
    ^scene_group

win.scene = am.group{am.depth_test("always", false) ^ bg, scene}
