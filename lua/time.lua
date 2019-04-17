am.delta_time = 0
am.frame_time = 0

local fps_window_size = 60
local fps_window_pos = 1
local fps_window = {}
local fps_prev_time = 0

am._register_pre_frame_func(function(dt, curr_time)
    am.delta_time = dt
    am.frame_time = am.frame_time + dt

    fps_window[fps_window_pos] = curr_time - fps_prev_time
    fps_prev_time = curr_time
    fps_window_pos = fps_window_pos % fps_window_size + 1
end)

function am.perf_stats()
    local stats = {}
    local total_time = 0.0000001
    local max_dt = 0.0000001
    local count = 0
    for i = 1, fps_window_size do
        if fps_window[i] then
            total_time = total_time + fps_window[i]
            if fps_window[i] > max_dt then
                max_dt = fps_window[i]
            end
            count = count + 1
        end
    end
    --log("total time = %0.5f, max_time = %0.5f", total_time, max_dt)
    stats.time = total_time
    stats.frames = count
    stats.min_fps = 1 / max_dt
    stats.avg_fps = count / total_time
    stats.frame_draw_calls = am._frame_draw_calls()
    stats.frame_use_program_calls = am._frame_use_program_calls()
    return stats
end

local perf_overlay_node

function am.show_perf_overlay()
    if not am._main_window then
        return
    end
    am.enable_perf_timings()
    local bar_w = 4
    local bar_h = 100
    local graph_h = bar_h
    local min_bar_h = 2
    local bar_t = 1/60
    local buf = 0
    local lua_graph_color = vec4(0.121, 0.317, 0.898, 0.85)
    local draw_graph_color = vec4(0.568, 0.321, 0.133, 0.85)
    local frame_graph_color = vec4(0.149, 0.509, 0.301, 0.85)
    local lua_text_color = vec4(0.741, 0.854, 1, 1)
    local draw_text_color = vec4(0.901, 0.729, 0.596, 1)
    local frame_text_color = vec4(0.525, 0.874, 0.674, 1)
    local fps60_line_color = vec4(0.984, 0.925, 0.137, 1)
    local bg_color = vec4(0, 0, 0, 0.75)
    if not perf_overlay_node then
        local win = am._main_window
        if not win._overlay then
            win._overlay = am.group()
        end
        local pw, ph = win.pixel_width, win.pixel_height
        local overlay_w = pw - buf * 2
        local num_bars = math.ceil(overlay_w / bar_w) + 1
        local num_quads = num_bars * 4
        local n = 0
        local bar_quads = am.quads(num_quads, {"vert", "vec2", "color", "vec4"})
        local graph_y1 = buf
        local graph_y2 = graph_y1 + graph_h
        local graph_x1 = buf
        local graph_x2 = graph_x1 + overlay_w
        local fps60_y = graph_y1 + bar_h
        local bar_translate = am.translate(0, 0) ^ bar_quads

        local annotations = am.group() ^ {
            am.line(vec2(graph_x1, fps60_y), vec2(graph_x2, fps60_y), 2, fps60_line_color),
            am.translate(graph_x1 + 5, fps60_y - 5) ^ am.text("0", frame_text_color, "left", "top"):tag"frame_t",
            am.translate(graph_x1 + 5, fps60_y - 25) ^ am.text("0", draw_text_color, "left", "top"):tag"draw_t",
            am.translate(graph_x1 + 5, fps60_y - 45) ^ am.text("0", lua_text_color, "left", "top"):tag"lua_t",
        }

        perf_overlay_node = am.depth_test("always", false)
            ^ am.bind{P = math.ortho(0, pw, 0, ph)}
            ^ am.use_program(am.shaders.colors2d)
            ^ am.blend"alpha"
            ^ {bar_translate, annotations}

        local prev_lua_y = graph_y1
        local prev_draw_y = graph_y1
        local prev_frame_y = graph_y1
        perf_overlay_node:action(function(node)
            if win:resized() then
                am.hide_perf_overlay()
                am.show_perf_overlay()
                return true
            end
            if n >= num_bars then
                bar_quads:remove_quad(1, 4)
            end
            local frame_t = am.delta_time
            local draw_t = am.last_frame_draw_time()
            local lua_t = am.last_frame_lua_time()
            local lua_h = math.max(min_bar_h, lua_t / bar_t * bar_h)
            local draw_h = math.max(min_bar_h, draw_t / bar_t * bar_h)
            local x1 = graph_x2 + (n-1) * bar_w
            local x2 = x1 + bar_w
            local lua_y = graph_y1 + lua_h
            local draw_y = lua_y + draw_h
            local frame_y = graph_y1 + frame_t / bar_t * bar_h
            local top_y = math.max(frame_y, graph_y2) + min_bar_h
            local prev_top_y = math.max(prev_frame_y, graph_y2) + min_bar_h
            if n == 1 then
                bar_quads:add_quad{
                    vert = {graph_x1, graph_y2, graph_x1, graph_y1, graph_x2, graph_y1, graph_x2, graph_y2},
                    color = bg_color,
                }
            else
                bar_quads:add_quad{
                    vert = {x1, prev_top_y, x1, prev_frame_y, x2, frame_y, x2, top_y},
                    color = bg_color,
                }
            end
            bar_quads:add_quad{
                vert = {x1, prev_lua_y, x1, graph_y1, x2, graph_y1, x2, lua_y},
                color = lua_graph_color,
            }
            bar_quads:add_quad{
                vert = {x1, prev_draw_y, x1, prev_lua_y, x2, lua_y, x2, draw_y},
                color = draw_graph_color,
            }
            bar_quads:add_quad{
                vert = {x1, prev_frame_y, x1, prev_draw_y, x2, draw_y, x2, frame_y},
                color = frame_graph_color,
            }
            prev_lua_y = lua_y
            prev_draw_y = draw_y
            prev_frame_y = frame_y
            n = n + 1
            bar_translate.x = graph_x2 - x1

            annotations"frame_t".text = string.format("FRAME:  %4dms", frame_t * 1000)
            annotations"draw_t".text =  string.format("RENDER: %4dms", draw_t * 1000)
            annotations"lua_t".text =   string.format("SCRIPT: %4dms", lua_t * 1000)
        end)
        win._overlay:append(perf_overlay_node)
    end
end

function am.hide_perf_overlay()
    if perf_overlay_node then
        if am._main_window and am._main_window._overlay then
            am._main_window._overlay:remove(perf_overlay_node)
        end
        perf_overlay_node = nil
    end
end

function am.toggle_perf_overlay()
    if perf_overlay_node then
        am.hide_perf_overlay()
    else
        am.show_perf_overlay()
    end
end
