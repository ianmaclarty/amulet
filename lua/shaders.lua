local color_v = [[
    precision highp float;
    attribute vec3 vert;
    uniform mat4 MV;
    uniform mat4 P;
    void main() {
        gl_Position = P * MV * vec4(vert, 1);
    }
]]

local color2d_v = [[
    precision highp float;
    attribute vec2 vert;
    uniform mat4 MV;
    uniform mat4 P;
    void main() {
        gl_Position = P * MV * vec4(vert, 0.0, 1.0);
    }
]]

local color_f = [[
    precision mediump float;
    uniform vec4 color;
    void main() {
        gl_FragColor = color;
    }
]]

local colors_v = [[
    precision highp float;
    attribute vec3 vert;
    attribute vec4 color;
    uniform mat4 MV;
    uniform mat4 P;
    varying vec4 v_color;
    void main() {
        v_color = color;
        gl_Position = P * MV * vec4(vert, 1);
    }
]]

local colors2d_v = [[
    precision highp float;
    attribute vec2 vert;
    attribute vec4 color;
    uniform mat4 MV;
    uniform mat4 P;
    varying vec4 v_color;
    void main() {
        v_color = color;
        gl_Position = P * MV * vec4(vert, 0.0, 1.0);
    }
]]

local colors_f = [[
    precision mediump float;
    varying vec4 v_color;
    void main() {
        gl_FragColor = v_color;
    }
]]

local texture_v = [[
    precision highp float;
    attribute vec3 vert;
    attribute vec2 uv;
    uniform mat4 MV;
    uniform mat4 P;
    varying vec2 v_uv;
    void main() {
        v_uv = uv;
        gl_Position = P * MV * vec4(vert, 1.0);
    }
]]

local texture2d_v = [[
    precision highp float;
    attribute vec2 vert;
    attribute vec2 uv;
    uniform mat4 MV;
    uniform mat4 P;
    varying vec2 v_uv;
    void main() {
        v_uv = uv;
        gl_Position = P * MV * vec4(vert, 0.0, 1.0);
    }
]]

local texture_f = [[
    precision mediump float;
    uniform sampler2D tex;
    varying vec2 v_uv;
    void main() {
        gl_FragColor = texture2D(tex, v_uv);
    }
]]

local texturecolor_f = [[
    precision mediump float;
    uniform sampler2D tex;
    uniform vec4 color;
    varying vec2 v_uv;
    void main() {
        gl_FragColor = texture2D(tex, v_uv) * color;
    }
]]

local premult_texturecolor_f = [[
    precision mediump float;
    uniform sampler2D tex;
    uniform vec4 color;
    varying vec2 v_uv;
    void main() {
        gl_FragColor = texture2D(tex, v_uv) * vec4(color.rgb * color.a, color.a);
    }
]]

local sources = {
    color = {color_v, color_f},
    color2d = {color2d_v, color_f},
    colors = {colors_v, colors_f},
    colors2d = {colors2d_v, colors_f},
    texture = {texture_v, texture_f},
    texture2d = {texture2d_v, texture_f},
    texturecolor = {texture_v, texturecolor_f},
    texturecolor2d = {texture2d_v, texturecolor_f},
    premult_texturecolor2d = {texture2d_v, premult_texturecolor_f},
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
