local am = amulet

local win = am.window{}

local audio_data_view
local audio_buffer
local audio_node
local num_samples
local shader_program
local editor_node

local
function create_audio_buffer()
    local rate = 44100
    local freq = 110
    num_samples = math.floor(rate / freq)
    audio_buffer = am.buffer(num_samples * 4)
    audio_data_view = audio_buffer:view("float", 0, 4)
    for i = 1, num_samples do
        audio_data_view[i] = math.sin(((i-1)/num_samples) * 2 * math.pi)
    end
end

local
function create_audio_node()
    audio_node = am.track(audio_buffer, true, 1, 1)
    am.root_audio_node():add(audio_node)
end

local
function create_shader_program()
    local vshader = [[
        precision mediump float;
        attribute float x;
        attribute float y;
        attribute vec3 color;
        uniform mat4 MVP;
        varying vec3 _color;
        void main() {
            _color = color;
            gl_Position = MVP * vec4(x, y, 0, 1);
        }
    ]]
    local fshader = [[
        precision mediump float;
        varying vec3 _color;
        void main() {
            gl_FragColor = vec4(_color, 1.0);
        }
    ]]
    shader_program = am.program(vshader, fshader)
end

local
function create_wave_editor()
    local xs = am.buffer(num_samples * 4):view("float", 0, 4)
    for i = 1, num_samples do
        xs[i] = ((i-1)/num_samples) * 2 - 1
    end
    local prev_index
    editor_node = am.draw_arrays("line_strip")
        :bind_array("x", xs)
        :bind_array("y", audio_data_view)
        :bind_vec3("color", vec3(0, 1, 0.6))
        :action(function(node)
            if am.mouse_button_down.left then
                local pos = am.mouse_position
                local index = math.floor(pos.x * num_samples * 0.5 + num_samples * 0.5) + 1
                if prev_index or index > 0 and index <= num_samples then
                    index = math.max(1, math.min(num_samples, index))
                    local lo, hi
                    if prev_index then
                        if prev_index < index then
                            lo = prev_index
                            hi = index
                        else
                            lo = index
                            hi = prev_index
                        end
                    else
                        lo = index
                        hi = index
                    end
                    for i = lo, hi do
                        audio_data_view[i] = pos.y
                    end
                    prev_index = index
                end
            else
                prev_index = nil
            end
            return 0
        end)
end

local
function assemble_ui()
    win.root = editor_node
        :bind_mat4("MVP")
        :bind_program(shader_program)
end
create_audio_buffer()
create_audio_node()
create_shader_program()
create_wave_editor()
assemble_ui()
