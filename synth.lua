-- click on the wave to edit it
-- the sliders control various filters and pitch
local win = am.window{}

local audio_data_view
local audio_buffer
local track_node
local lowpass_filter_node
local highpass_filter_node
local num_samples
local shader_program
local wave_editor_node
local pitch_editor_node
local lowpass_cutoff_editor_node
local lowpass_resonance_editor_node
local highpass_cutoff_editor_node
local highpass_resonance_editor_node

local scene = am.group()
win.scene = scene

local
function create_audio_buffer()
    local rate = 44100
    local freq = 440
    num_samples = math.floor(rate / freq)
    local raw_buffer = am.buffer(num_samples * 4)
    audio_data_view = raw_buffer:view("float", 0, 4)
    for i = 1, num_samples do
        local val = math.sin(((i-1)/num_samples) * 2 * math.pi)
        audio_data_view[i] = val
    end
    audio_buffer = am.audio_buffer(raw_buffer, 1, rate)
end

local
function create_audio_node()
    track_node = am.track(audio_buffer, true)
    lowpass_filter_node = track_node:lowpass_filter(2000, 0)
    highpass_filter_node = lowpass_filter_node:highpass_filter(20, 0)
    scene:action(am.play(highpass_filter_node))
end

local
function create_shader_program()
    local vshader = [[
        precision mediump float;
        attribute float x;
        attribute float y;
        uniform mat4 MVP;
        void main() {
            gl_Position = MVP * vec4(x, y, 0, 1);
        }
    ]]
    local fshader = [[
        precision mediump float;
        uniform vec3 color;
        void main() {
            gl_FragColor = vec4(color, 1.0);
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
    local graph = 
        am.read_uniform("MVP")
            :action(function(node)
                if win:mouse_down("left") then
                    local pos = math.inverse(node.value) * vec4(win:mouse_norm_position(), 0, 1)
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
                                local val = y1 + ((i-lo)/(hi-lo))*(y2-y1)
                                audio_data_view[i] = val
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
            end)
        ^am.bind{
            x = xs,
            y = audio_data_view,
            color = vec3(0, 1, 0.6),
        }
        ^am.draw("line_strip")
    local line = 
        am.bind{
            x = am.float_array{-1, 1},
            y = am.float_array{ 0, 0},
            color = vec3(0.5, 0.5, 0.5),
        }
        ^am.draw("lines")
    local border = 
        am.bind{
            x = am.float_array{-1, -1,  1,  1},
            y = am.float_array{-1,  1,  1, -1},
            color = vec3(1, 0.5, 0.5),
        }
        ^am.draw("line_loop")
    wave_editor_node = am.group(line, graph, border)
end

local
function create_slider(min, max, initial_value, onchange)
    local grip_w = 0.1
    local grip_h = 0.2
    local grip = 
        am.translate("MVP", vec3(((initial_value-min)/(max-min))*2-1, 0, 0))
        ^am.bind{
            x = am.float_array{-grip_w/2, grip_w/2, grip_w/2, -grip_w/2},
            y = am.float_array{-grip_h/2, -grip_h/2, grip_h/2, grip_h/2},
            color = vec3(0.8, 0.8, 0.2),
        }
        ^am.draw("line_loop")
    local line = 
        am.bind{
            x = am.float_array{-1, 1},
            y = am.float_array{ 0, 0},
            color = vec3(0.5, 0.5, 1),
        }
        ^am.draw("lines")
    local prev_pos
    local inv
    return 
        am.read_uniform("MVP")
            :action(function(node)
                if not prev_pos and win:mouse_pressed("left") then
                    inv = math.inverse(node.value)
                    local pos = inv * vec4(win:mouse_norm_position(), 0, 1)
                    if math.abs(pos.x) <= 1+grip_w/2 and math.abs(pos.y) < grip_h/2 then
                        prev_pos = pos
                    end
                end
                if prev_pos and win:mouse_down("left") then
                    local pos = inv * vec4(win:mouse_norm_position(), 0, 1)
                    local x = math.min(1, math.max(-1, pos.x))
                    grip.position = vec3(x, 0, 0)
                    onchange(min + (x * 0.5 + 0.5) * (max - min))
                    prev_pos = pos
                else
                    prev_pos = nil
                end
            end)
        ^am.group(line, grip)
end

local
function create_pitch_editor()
    local max_pitch = 4
    pitch_editor_node = create_slider(-1, 1, 0, function(v)
        track_node.playback_speed = 2^(v * max_pitch)
    end)
end

local
function create_lowpass_cutoff_editor()
    lowpass_cutoff_editor_node = create_slider(20, 2000, 2000, function(v)
        lowpass_filter_node.cutoff = v
    end)
end

local
function create_lowpass_resonance_editor()
    lowpass_resonance_editor_node = create_slider(0, 50, 0, function(v)
        lowpass_filter_node.resonance = v
    end)
end

local
function create_highpass_cutoff_editor()
    highpass_cutoff_editor_node = create_slider(20, 2000, 20, function(v)
        highpass_filter_node.cutoff = v
    end)
end

local
function create_highpass_resonance_editor()
    highpass_resonance_editor_node = create_slider(0, 50, 0, function(v)
        highpass_filter_node.resonance = v
    end)
end

local
function assemble_ui()
    local dy = 0.15
    local x = 0.25
    scene:append(
        am.use_program(shader_program)
            :action(function()
                if win:key_down("escape") then
                    win:close()
                end
            end)
        ^am.bind{MVP = mat4(1)}
        ^{
            am.translate("MVP", vec3(x, -0.43, 0))
            ^am.scale("MVP", vec3(0.7, 0.45, 1))
            ^wave_editor_node
            ,
            am.translate("MVP", vec3(x, 0.2, 0))
            ^am.scale("MVP", vec3(0.7, 0.5, 1))
            ^pitch_editor_node
            ,
            am.translate("MVP", vec3(x, 0.2+dy, 0))
            ^am.scale("MVP", vec3(0.7, 0.5, 1))
            ^lowpass_cutoff_editor_node
            ,
            am.translate("MVP", vec3(x, 0.2+2*dy, 0))
            ^am.scale("MVP", vec3(0.7, 0.5, 1))
            ^lowpass_resonance_editor_node
            ,
            am.translate("MVP", vec3(x, 0.2+3*dy, 0))
            ^am.scale("MVP", vec3(0.7, 0.5, 1))
            ^highpass_cutoff_editor_node
            ,
            am.translate("MVP", vec3(x, 0.2+4*dy, 0))
            ^am.scale("MVP", vec3(0.7, 0.5, 1))
            ^highpass_resonance_editor_node
            ,
            am.translate(-310, -10) ^ am.text("WAVE:\n(CLICK TO EDIT)", "left")
            ,
            am.translate(-310, 50) ^ am.text("PITCH:", "left")
            ,
            am.translate(-310, 90) ^ am.text("LOW PASS FREQ:", "left")
            ,
            am.translate(-310, 122) ^ am.text("LOW PASS RES:", "left")
            ,
            am.translate(-310, 160) ^ am.text("HIGH PASS FREQ:", "left")
            ,
            am.translate(-310, 195) ^ am.text("HIGH PASS RES:", "left")
        }
    )
end

create_audio_buffer()
create_audio_node()
create_shader_program()
create_wave_editor()
create_pitch_editor()
create_lowpass_cutoff_editor()
create_lowpass_resonance_editor()
create_highpass_cutoff_editor()
create_highpass_resonance_editor()
assemble_ui()
