
# Shader programs {#shader-programs}

## Compiling a shader program

### am.program(vertex_shader, fragment_shader) {#am.program .func-def}

Compiles and returns a shader program for use with
[`am.use_program`](#am.use_program) nodes.

`vertex_shader` and `fragment_shader` should be the sources for the vertex and
fragment shaders in the OpenGL ES shader language version 1.  This is the same
shader language supported by WebGL 1.

Example:

~~~ {.lua}
local vert_shader = [[
    precision highp float;
    attribute vec3 vert;
    uniform mat4 MV;
    uniform mat4 P;
    void main() {
        gl_Position = P * MV * vec4(vert, 1);
    }
]]
local frag_shader = [[
    precision mediump float;
    void main() {
        gl_FragColor = vec4(1.0, 0, 0.5, 1.0);
    }
]]
local prog = am.program(vert_shader, frag_shader)
~~~
