
# 3D models

Amulet has some basic support for loading 3D models in Wavefront `.obj` format.

### am.load_obj(filename) {#am.load_obj .func-def}

This loads the given `.obj` file and returns 4 things:

1. A buffer containing the vertex, normal and texture coordinate
   data.
2. The stride in bytes.
3. The offset of the normals in bytes.
4. The offset of the texture coordinates in bytes.

The vertex data is always at offset 0. If the normal or texture coordinate data
is not present, the corresponding return value will be nil.

The faces in the `.obj` file must all be triangles (quads aren't supported).

Here's an example of how to load a model and display it. The example
loads an model from `model.obj` and assumes it contains normal
and texture coordinate data and the triangles have a counter-clockwise
winding. It loads a texture from the file `texture.png`.

~~~ {.lua}
local win = am.window{depth_buffer = true}

local buf, stride, norm_offset, tex_offset = am.load_obj("model.obj")
local verts = buf:view("vec3", 0, stride)
local normals = buf:view("vec3", norm_offset, stride)
local uvs = buf:view("vec2", tex_offset, stride)
    
local shader = am.program([[
precision mediump float;
attribute vec3 vert;
attribute vec2 uv;
attribute vec3 normal;
uniform mat4 MV;
uniform mat4 P;
varying vec3 v_shadow;
varying vec2 v_uv;
void main() {
    vec3 light = normalize(vec3(1, 0, 2));
    vec3 nm = normalize((MV * vec4(normal, 0.0)).xyz);
    v_shadow = vec3(max(0.1, dot(light, nm)));
    v_uv = uv;
    gl_Position = P * MV * vec4(vert, 1.0);
}
]], [[
precision mediump float;
uniform sampler2D tex;
varying vec3 v_shadow;
varying vec2 v_uv;
void main() {
    gl_FragColor = texture2D(tex, v_uv) * vec4(v_shadow, 1.0);
}
]])

win.scene =
    am.cull_face"ccw"
    ^ am.translate(0, 0, -5)
    ^ am.use_program(shader)
    ^ am.bind{
        P = math.perspective(math.rad(60), win.width/win.height, 1, 1000),
        vert = verts,
        normal = normals,
        uv = uvs,
        tex = am.texture2d("texture.png"),
    }
    ^am.draw"triangles"
~~~
