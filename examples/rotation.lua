local win = am.window{depth_buffer = true, msaa_samples = 8}

math.randomseed(os.time())

local vshader = [[
precision mediump float;
attribute vec3 pos;
attribute vec3 normal;
uniform mat4 MV;
uniform mat4 P;
varying vec3 v_color;
void main() {
    vec3 light = normalize(vec3(0.1, -0.5, 1.0));
    vec3 nm = normalize((MV * vec4(normal, 0.0)).xyz);
    v_color = vec3(max(0.3, dot(light, nm)));
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
    return 
        am.bind{
            pos = verts,
            normal = normals,
        }
        ^am.draw[[triangles]]
end

local cube = load_model("cube.obj")

local rotating_cube = 
    am.rotate("MV", quat(0))
        :action(coroutine.create(function(node)
            while true do
                am.wait(am.delay(1.0))
                local angle = math.random() * 2 * math.pi
                local axis = math.normalize(vec3(0, 0, 1))
                am.wait(am.tween(2, {rotation = quat(angle, axis)},
                    am.ease.inout(am.ease.cubic)), node)
            end
        end))
    ^cube

local scene_group = am.group()

local translated_cube = am.translate(0, 0, -1) ^ am.scale(vec3(0.5)) ^ rotating_cube

--local projection_matrix = math.perspective(math.rad(85), 1, 0.1, 10)
local oblique_matrix = math.oblique(-math.pi/2, 2.5, 0.5, -4, 4, -3, 3, 0, 20)
local ortho_matrix = math.ortho(-4, 4, -3, 3, 0, 20)

scene_group:append(translated_cube)

local scene = 
    am.use_program(shader)
        :action(function()
            if win:key_pressed("escape") then
                win:close()
            end
        end)
    ^ {
        am.bind{
            MV = mat4(1),
            P = oblique_matrix,
        },
        --[[
        am.bind{
            MV = mat4(1),
            P = ortho_matrix,
        },
        ]]
    }
    ^ scene_group

win.scene = scene
