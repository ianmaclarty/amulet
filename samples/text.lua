local am = amulet

local win = am.window{width = 1000, height = 480}

local vera_mono = am.load_font("VeraMono.ttf", 480)

local img = am.create_image(win.width, win.height)

vera_mono:render_text(img, vec2(0, 0), "Hello $123.456 !@#$%^&*()_-+={} \"';:.,<>?/\\|~`")

local texture = am.texture2d{
    width = img.width,
    height = img.height,
    buffer = img.buffer,
}

local shader = am.program([[
precision mediump float;
attribute vec2 pos;
attribute vec2 uv;
varying vec2 v_uv;
void main() {
    v_uv = uv;
    gl_Position = vec4(pos, 0, 1);
}
]], [[
precision mediump float;
varying vec2 v_uv;
uniform sampler2D tex;
void main() {
    gl_FragColor = texture2D(tex, v_uv);
}
]])

win.root = am.draw_elements(am.ushort_elem_array{1, 2, 3, 3, 2, 4})
    :bind_array("pos", am.vec2_array{-1, -1, 1, -1, -1, 1, 1, 1})
    :bind_array("uv", am.vec2_array{0, 0, 1, 0, 0, 1, 1, 1})
    :bind_sampler2d("tex", texture)
    :bind_program(shader)
