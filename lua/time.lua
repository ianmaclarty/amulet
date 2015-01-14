amulet.delta_time = 0
amulet.frame_time = 0

local fps_window_size = 60
local fps_window_pos = 1
local fps_window = {}
local fps_prev_time = 0

function amulet._update_times(dt, curr_time)
    amulet.delta_time = dt
    amulet.frame_time = amulet.frame_time + dt

    fps_window[fps_window_pos] = curr_time - fps_prev_time
    fps_prev_time = curr_time
    fps_window_pos = fps_window_pos % fps_window_size + 1
end

function amulet.perf_stats()
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
    stats.min_fps = 1 / max_dt
    stats.avg_fps = count / total_time
    return stats
end
