win = am.window{title = "Sounds"}

group = am.group()

for i = 0, 11 do
    for j = 0, 3 do
        local x1 = 640 / 12 * i - 320
        local y1 = 480 / 4 * j - 240
        local x2 = x1 + 640 / 12
        local y2 = y1 + 480 / 4
        local color = vec4(1-i/12, i/12, j/5 + 0.2, 1)
        local rect = am.rect(x1, y1, x2, y2, color)
        local playing = false
        rect:action(function()
            local pos = win:mouse_position()
            if pos.x > x1 and pos.x < x2 and pos.y > y1 and pos.y < y2 then
                if win:mouse_pressed"left" or not playing and win:mouse_down"left" then
                    playing = true
                    local pitch = 2 ^ (i / 12 - 3 + j)
                    rect.color = vec4(1 - color.rgb, 1)
                    rect:action("sound", am.series{
                        am.play(55581500, false, pitch),
                        function()
                            rect.color = color
                            playing = false
                            return true
                        end
                    })
                end
            end
        end)
        group:append(rect)
    end
end

win.scene = group
