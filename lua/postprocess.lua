-- settings:
--      width
--      height
--      minfilter
--      magfilter
--      depth_buffer
--      stencil_buffer
--      clear_color
--      auto_clear
--      program
--      bindings
function am.postprocess(opts)
    local main_window = am._main_window
    local w, h
    local track_window = false
    if opts.width and opts.height then
        w, h = opts.width, opts.height
        track_window = false
    elseif main_window then
        w, h = main_window.pixel_width, main_window.pixel_height
        track_window = true
    else
        error("no width and height given and no window created", 2)
    end
    local minfilter, magfilter = opts.minfilter or "nearest", opts.magfilter or "nearest"
    local tex = am.texture2d(w, h)
    tex.minfilter = minfilter
    tex.magfilter = magfilter
    local fb = am.framebuffer(tex, opts.depth_buffer, opts.stencil_buffer)
    if opts.clear_color then
        fb.clear_color = opts.clear_color
    end
    fb:clear()
    local auto_clear = true
    if opts.auto_clear == false then
        auto_clear = false
    end
    local program = opts.program or am.shaders.texture2d
    local user_bindings = opts.bindings or {}
    local pp_bindings = {
        P = mat4(1),
        uv = am.rect_verts_2d(0, 0, 1, 1),
        vert = am.rect_verts_2d(-1, -1, 1, 1),
        tex = tex,
    }
    for k, v in pairs(user_bindings) do
        pp_bindings[k] = v
    end
    local node =
        am.use_program(program)
        ^ am.bind(pp_bindings)
        ^ am.draw("triangles", am.rect_indices())
    local wrap = am.wrap(node)
    fb.projection = main_window.projection
    wrap:action(function()
        if track_window then
            if main_window:resized() then
                w, h = main_window.pixel_width, main_window.pixel_height
                fb:resize(w, h)
            end
        end
        fb.projection = main_window.projection
        if auto_clear then
            fb:clear()
        end
        fb:render_children(wrap)
    end, 1000)
    function wrap:get_clear_color()
        return fb.clear_color
    end
    function wrap:set_clear_color(cc)
        fb.clear_color = cc
    end
    function wrap:get_auto_clear()
        return auto_clear
    end
    function wrap:set_auto_clear(ac)
        auto_clear = ac
    end
    function wrap:get_program()
        return program
    end
    function wrap:set_program(p)
        program = p
        node"use_program".program = p
    end
    function wrap:get_bindings()
        return pp_bindings
    end
    function wrap:set_bindings(b)
        pp_bindings = b
        node"bind".bindings = b
    end
    function wrap:clear()
        fb:clear()
    end
    wrap:tag"postprocess"
    return wrap
end
