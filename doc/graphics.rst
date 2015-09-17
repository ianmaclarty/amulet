Graphics
========

Overview
--------

Creating a window
-----------------

Shaders
-------

Scene graphs
------------

In Amulet, all drawing is done by constructing a
*scene graph* which is then rendered to a window.
Each node in the scene graph either configures some part of
the graphics system (for example setting the blend mode or
shader program), or draws some graphics primitives, such
as triangles or lines.

Here is an example scene graph that draws a single red triangle:

..  graphviz::

    digraph {
        node [shape="box", fontname="Courier bold", fontsize="12", fontcolor="#494149", color="#494149", penwidth=1];
        edge [color="#494149"];
        use_program [label="use_program(am.shaders.color2d)"]
        bind [label="bind{\l  P = mat4(1),\l  MV = mat4(1),\l  color = vec3(1, 0, 0, 1),\l  pos = am.vec2_array{-1, -1, 0, 1, 1, -1}\l}\l"]
        draw_triangles [label="draw_triangles{1, 2, 3}"]
        use_program -> bind -> draw_triangles
    }

This scene graph has 3 nodes:

1.  The root node, ``use_program``, sets up the shader
    program to use for subsequent rendering. Here we use the built-in shader program
    ``am.shaders.color2d`` which is a simple shader that renders 2D vertices using a
    single color.
2.  The ``bind`` node binds values to the shader's
    uniforms and attributes -- it supplies data to the shader.
    The ``am.shaders.color2d`` shader expects perspective and modelview
    matrix uniforms called ``P`` and ``MV`` respectively. We bind these
    to the identity matrices. It expects a vec4 uniform called ``color``
    which we bind to ``vec4(1, 0, 0, 1)`` (red). The shader also expects
    a vec2 attribute called ``pos``. We bind this to a vec2 array which
    we construct using the handy ``am.vec2_array`` function. The array
    contains 3 vertices: (-1, -1), (1, -1) and (0, 1). Since we're
    using the identity perspective and modelview matrices these will
    correspond to the bottom left, bottom right and top middle points of
    the window we're drawing to.
3.  Finally the ``draw_triangles`` node draws triangle primitives using the
    previously set up shader and data. We supply a list of vertex IDs to
    describe the triangles we want to draw. In this case we've only
    supplied 3 vertices, so we use them. (A vertex ID is just the position
    of the vertex in the bound array, starting at 1).

Binding shader inputs
---------------------

Drawing primitives
------------------

Transforms
----------

Textures
--------

Framebuffers
------------

Graphics API reference
----------------------
