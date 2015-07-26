local am = amulet

local color_v = [[
    precision highp float;
    attribute vec3 pos;
    uniform mat4 MV;
    uniform mat4 P;
    void main() {
        gl_Position = P * MV * vec4(pos, 1);
    }
]]

local color_f = [[
    precision mediump float;
    uniform vec4 color;
    void main() {
        gl_FragColor = color;
    }
]]

local sources = {
    color = {color_v, color_f}
}

am.shaders = {}

setmetatable(am.shaders, {
    -- compile shaders lazily
    __index = function(shaders, name)
        local prog = rawget(shaders, name)
        if not prog then
            if not sources[name] then
                return nil
            end
            prog = am.program(sources[name][1], sources[name][2])
            rawset(shaders, name, prog)
        end
        return prog
    end
})
