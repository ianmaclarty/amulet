local win = am.window{}

local green = vec4(0, 1, 0, 1)
local blue = vec4(0, 0, 1, 1)
local yellow = vec4(1, 1, 0, 1)
local red = vec4(1, 0, 0, 1)
local dark_green = vec4(0, 0.5, 0, 1)
local dark_blue = vec4(0, 0, 0.5, 1)
local dark_yellow = vec4(0.5, 0.5, 0, 1)
local dark_red = vec4(0.5, 0, 0, 1)
local white = vec4(1, 1, 1, 1)
local grey = vec4(0.5, 0.5, 0.5, 1)

local
function controller_view(index)
    local lstick_node = am.translate(-100, 50) ^ am.rect(-10, -10, 10, 10, grey)
    local rstick_node = am.translate(100, 50) ^ am.rect(-10, -10, 10, 10, grey)
    local a_button_node = am.translate(100, -75) ^ am.rect(-10, -10, 10, 10, dark_green)
    local b_button_node = am.translate(125, -50) ^ am.rect(-10, -10, 10, 10, dark_red)
    local x_button_node = am.translate(75, -50) ^ am.rect(-10, -10, 10, 10, dark_blue)
    local y_button_node = am.translate(100, -25) ^ am.rect(-10, -10, 10, 10, dark_yellow)
    local down_button_node = am.translate(-100, -75) ^ am.rect(-10, -10, 10, 10, grey)
    local left_button_node = am.translate(-125, -50) ^ am.rect(-10, -10, 10, 10, grey)
    local right_button_node = am.translate(-75, -50) ^ am.rect(-10, -10, 10, 10, grey)
    local back_button_node = am.translate(-30, 50) ^ am.rect(-15, -10, 15, 10, grey)
    local start_button_node = am.translate(30, 50) ^ am.rect(-15, -10, 15, 10, grey)
    local up_button_node = am.translate(-100, -25) ^ am.rect(-10, -10, 10, 10, grey)
    local lb_node = am.translate(-100, 125) ^ am.rect(-30, -5, 30, 5, grey)
    local rb_node = am.translate(100, 125) ^ am.rect(-30, -5, 30, 5, grey)
    local lt_node = am.translate(-100, 200) ^ am.rect(-20, -5, 20, 5, grey)
    local rt_node = am.translate(100, 200) ^ am.rect(-20, -5, 20, 5, grey)

    local controller_node = am.group{
        lstick_node, rstick_node,
        a_button_node, b_button_node, x_button_node, y_button_node,
        left_button_node, right_button_node, up_button_node, down_button_node,
        lb_node, rb_node,
        lt_node, rt_node,
        back_button_node,
        start_button_node,
    }
    local not_active_msg = am.scale(2) ^ am.text("Controller "..index.."\nnot attached")
    local scene = am.group{
        controller_node,
        not_active_msg,
    }
    scene:action(function()
        if am.controller_present(index) then
            controller_node.hidden = false
            not_active_msg.hidden = true
            lstick_node.position2d = am.controller_lstick_pos(index) * 50 + vec2(-100, 50)
            rstick_node.position2d = am.controller_rstick_pos(index) * 50 + vec2(100, 50)
            lstick_node"rect".color = am.controller_button_down(index, "ls") and white or grey
            rstick_node"rect".color = am.controller_button_down(index, "rs") and white or grey
            a_button_node"rect".color = am.controller_button_down(index, "a") and green or dark_green
            b_button_node"rect".color = am.controller_button_down(index, "b") and red or dark_red
            x_button_node"rect".color = am.controller_button_down(index, "x") and blue or dark_blue
            y_button_node"rect".color = am.controller_button_down(index, "y") and yellow or dark_yellow
            lb_node"rect".color = am.controller_button_down(index, "lb") and white or grey
            rb_node"rect".color = am.controller_button_down(index, "rb") and white or grey
            lt_node.y = 200 - am.controller_lt_val(index) * 50
            rt_node.y = 200 - am.controller_rt_val(index) * 50
            left_button_node"rect".color = am.controller_button_down(index, "left") and white or grey
            right_button_node"rect".color = am.controller_button_down(index, "right") and white or grey
            up_button_node"rect".color = am.controller_button_down(index, "up") and white or grey
            down_button_node"rect".color = am.controller_button_down(index, "down") and white or grey
            back_button_node"rect".color = am.controller_button_down(index, "back") and white or grey
            start_button_node"rect".color = am.controller_button_down(index, "start") and white or grey
        else
            controller_node.hidden = true
            not_active_msg.hidden = false
        end
    end)

    return scene
end

local scene = am.group{
    am.translate(-160, 80) ^ am.scale(0.7) ^ controller_view(1),
    am.translate(160, 80) ^ am.scale(0.7) ^ controller_view(2),
    am.translate(-160, -150) ^ am.scale(0.7) ^ controller_view(3),
    am.translate(160, -150) ^ am.scale(0.7) ^ controller_view(4),
}

win.scene = scene

scene:action(function()
    if win:key_pressed"escape" then
        win:close()
    end
end)
