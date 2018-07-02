local win = am.window{}

-- shows how to use a transform node to achieve the same effect as a chain of 2
-- translate nodes, a scale node and a rotate node

local node1 = 
    am.translate(50, 0) 
    ^ am.rotate(math.rad(45)) 
    ^ am.scale(4) 
    ^ am.translate(0, 20)
    ^ am.text("TRANSFORMED", vec4(1, 0, 0, 1))

local mat = math.translate4(vec3(50, 0, 0)) 
    * mat4(quat(math.rad(45))) 
    * math.scale4(vec3(4, 4, 1)) 
    * math.translate4(vec3(0, 20, 0))

local node2 =
    am.transform(mat)
    ^ am.text("TRANSFORMED", vec4(0, 1, 0, 1))

win.scene = am.group()

win.scene:action(function()
    win.scene:remove_all()
    win.scene:append(math.fract(am.frame_time) < 0.5 and node1 or node2)
end)
