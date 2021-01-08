local win = am.window{
    title = "Beach",
    width = 400,
    height = 300,
}

win.scene = 
    am.group()
    ^ {
        am.sprite"beach.jpg"
        ,
        am.translate(0, -60):tag"ballt"
        ^ am.rotate(0):tag"ballr"
        ^ am.sprite"ball.png"
    }

local ball_pos = vec2(0, -60)
local ball_angle = 0
local velocity = vec2(0)
local spin = 0
local min_pos = vec2(-180, -60)
local max_pos = vec2(180, 500)
local min_v = vec2(-50, 150)
local max_v = vec2(50, 300)
local gravity = vec2(0, -500)
local on_ground = true

win.scene:action(function(scene)
    -- check if the left mouse button was pressed
    if win:mouse_pressed"left" then
        local mouse_pos = win:mouse_position()
        -- check if the mouse click is on the ball
        if math.distance(mouse_pos, ball_pos) < 50 then
            -- compute a velocity based on click position
            local dir = math.normalize(ball_pos - mouse_pos)
            velocity = dir * 300
            velocity = math.clamp(velocity, min_v, max_v)
            -- set a random spin
            spin = math.random() * 4 - 2
            -- play bounce sound
            scene:action(am.play("bounce.ogg"))
            on_ground = false
        end
    end

    -- update the ball position
    ball_pos = ball_pos + velocity * am.delta_time

    -- if the ball lands on the ground, set the
    -- velocity and spin to zero.
    if ball_pos.y <= -60 and not on_ground then
        velocity = vec2(0)
        spin = 0
        -- play land sound
        scene:action(am.play("land.ogg"))
        on_ground = true
    end

    -- clamp the ball position so it doesn't dissapear
    -- off the edge of the screen
    ball_pos = math.clamp(ball_pos, min_pos, max_pos)

    -- update the ball angle
    ball_angle = ball_angle + spin * am.delta_time

    -- update the ball translate and rotate nodes
    scene"ballt".position2d = ball_pos
    scene"ballr".angle = ball_angle

    -- apply gravity to the velocity
    velocity = velocity + gravity * am.delta_time
end)

-- play ocean sound in a loop
win.scene:action(am.play("ocean.ogg", true))
