local am = amulet

local win = am.window{}

local audio_data_view
local audio_buffer
local audio_node
local num_samples
local shader_program
local wave_editor_node
local pitch_editor_node

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
    local prev_y
    local graph = am.draw_arrays("line_strip")
        :bind_array("x", xs)
        :bind_array("y", audio_data_view)
        :bind_vec3("color", vec3(0, 1, 0.6))
        :read_mat4("MVP")
        :action(function(node)
            if am.mouse_button_down.left then
                local pos = math.inverse(node.value) * vec4(am.mouse_position, 0, 1)
                local index = math.floor(pos.x * num_samples * 0.5 + num_samples * 0.5) + 1
                if math.abs(pos.y) <= 1 and (prev_index or index > 0 and index <= num_samples) then
                    index = math.max(1, math.min(num_samples, index))
                    local lo, hi, y1, y2
                    if prev_index then
                        if prev_index < index then
                            lo = prev_index
                            hi = index
                            y1 = prev_y
                            y2 = pos.y
                        else
                            lo = index
                            hi = prev_index
                            y1 = pos.y
                            y2 = prev_y
                        end
                    else
                        lo = index
                        hi = index
                        y1 = pos.y
                        y2 = pos.y
                    end
                    if lo < hi then
                        for i = lo, hi do
                            audio_data_view[i] = y1 + ((i-lo)/(hi-lo))*(y2-y1)
                        end
                    else
                        audio_data_view[index] = y1
                    end
                    prev_index = index
                    prev_y = pos.y
                end
            else
                prev_index = nil
                prev_y = nil
            end 
            return 0
        end)
    local line = am.draw_arrays("lines")
        :bind_array("x", am.float_array{-1, 1})
        :bind_array("y", am.float_array{ 0, 0})
        :bind_vec3("color", vec3(0.5, 0.5, 0.5))
    local border = am.draw_arrays("line_loop")
        :bind_array("x", am.float_array{-1, -1,  1,  1})
        :bind_array("y", am.float_array{-1,  1,  1, -1})
        :bind_vec3("color", vec3(1, 0.5, 0.5))
    wave_editor_node = am.group(line, graph, border)
end

local
function create_pitch_editor()
    local grip_w = 0.1
    local grip_h = 0.2
    local max_pitch = 4
    local grip = am.draw_arrays("line_loop")
        :bind_array("x", am.float_array{-grip_w/2, grip_w/2, grip_w/2, -grip_w/2})
        :bind_array("y", am.float_array{-grip_h/2, -grip_h/2, grip_h/2, grip_h/2})
        :bind_vec3("color", vec3(0.8, 0.8, 0.2))
        :translate("MVP", 0):alias("position")
    local line = am.draw_arrays("lines")
        :bind_array("x", am.float_array{-1, 1})
        :bind_array("y", am.float_array{ 0, 0})
        :bind_vec3("color", vec3(0.5, 0.5, 0.5))
    local prev_pos
    pitch_editor_node = am.group(line, grip)
        :read_mat4("MVP")
        :action(function(node)
            if am.mouse_button_down.left then
                local pos = math.inverse(node.value) * vec4(am.mouse_position, 0, 1)
                if prev_pos or math.abs(pos.y) <= grip_h and math.abs(pos.x) <= 1 then
                    local x = math.min(1, math.max(-1, pos.x))
                    grip.position.x = x
                    audio_node.playback_speed = 2^(x * max_pitch)
                    prev_pos = pos
                end
            else
                prev_pos = nil
            end
            return 0
        end)
end

local
function assemble_ui()
    win.root = am.group(
            wave_editor_node
                :scale("MVP", 0.7, 0.5)
                :translate("MVP", 0.28, -0.48),
            pitch_editor_node
                :scale("MVP", 0.7, 0.5)
                :translate("MVP", 0.28, 0.2)
        )
        :bind_mat4("MVP")
        :bind_program(shader_program)
end

create_audio_buffer()
create_audio_node()
create_shader_program()
create_wave_editor()
create_pitch_editor()
assemble_ui()
