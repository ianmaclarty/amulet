Advanced graphics
=================

Overview
--------

This section goes into detail about Amulets low-level graphics features.
Some knowledge of how shaders and the modern graphics pipeline work is
recommended before reading this section. If you're just interested
in drawing simple sprites and shapes then you can probably skip this section,
but if you're interested in experimenting with shaders and 3D rendering
then read on.

.. _win-create:

Creating a window
-----------------

To create a window use the :func:`am.window` function. This function
accepts a table of window settings, all of which are optional.
(For a full list of settings see :func:`am.window` in the API reference.)

Here's how to create a 800x600 window:

..  code-block:: lua

    local win = am.window{
        title = "My Window",
        width = 800,
        height = 600,
    }

The ``width`` and ``height`` settings are just hints to the
operating system. It may create a bigger or smaller window.

Once a window is created you can obtain its actual pixel
dimensions using the ``width`` and ``height`` fields of the
window object. Don't assume these will be the same as the
width and height you requested when creating the window.

Also don't expect these values to remain constant, even
if you request a fullscreen window. On some
platforms the operating system will perform an animation
when transitioning to fullscreen mode and the width and
height fields will only be updated once the animation has
finished.

The :func:`window:resized` method can be used to detect if a window's
size changed since the last frame.

To close a window call its :func:`window:close` method. This also
quits the application if there was only one window open.

..  note::

    Windows are automatically cleared each frame before drawing.
    If you'd like for this not to happend, then you need to draw into an
    offscreen framebuffer and then draw that to the window instead.
    See :ref:`framebuffers` for more information.

Shaders
-------

Scene graphs
------------

In Amulet, all drawing is done by building a
*scene graph* that is then rendered to a window or
offscreen framebuffer.
Each node in the scene graph either configures some part of
the graphics system (for example setting the blend mode or
shader program), or draws some graphics primitives, such
as triangles or lines.

Here is an example scene graph that draws a colored triangle:

..  graphviz::

    digraph {
        node [shape="box", fontname="Courier bold", fontsize="12", fontcolor="#494149", color="#494149", penwidth=1];
        edge [color="#494149"];
        use_program [label="am.use_program(am.shaders.colors2d)"]
        bind [label="am.bind{\l  P = mat4(1),\l  MV = mat4(1),\l  color = am.vec4_array{vec4(1, 0, 0, 1), vec4(0, 1, 0, 1), vec4(0, 0, 1, 1)},\l  vert = am.vec2_array{vec2(-1, -1), vec2(0, 1), vec2(1, -1)}\l}\l"]
        draw [label="am.draw(\"triangles\")"]
        use_program -> bind -> draw
    }

This scene graph has 3 nodes:

1.  The root node, ``am.use_program``, sets up the shader
    program to use for subsequent rendering. Here we use the built-in shader program
    ``am.shaders.colors2d`` which is a simple shader that renders 2D vertices with
    a separate color per vertex.
2.  The ``am.bind`` node binds values to the shader's
    uniforms and attributes -- it supplies data to the shader.
    The ``am.shaders.colors2d`` shader expects perspective and modelview
    matrix uniforms called ``P`` and ``MV`` respectively. We bind these
    to the identity matrices. It expects a vec4 attribute called ``color``
    which we bind to an array of 3 colors (red, green and blue).
    The shader also expects
    a vec2 attribute called ``vert``. We bind this to a vec2 array 
    containing 3 vertices: (-1, -1), (1, -1) and (0, 1). Since we're
    using the identity perspective and modelview matrices these will
    correspond to the bottom left, bottom right and top middle points of
    the window we're drawing to.
3.  Finally the ``am.draw`` node draws triangle primitives using the
    previously set up shader and data.

The produced image looks like this:

..  figure:: screenshots/rgb_triangle.png
    :alt: 

And here's what the corresponding code looks like:

..  code-block:: lua

    am.window{}.root =
        am.use_program(am.shaders.colors2d)
        ^ am.bind{
            P = mat4(1),
            MV = mat4(1),
            color = am.vec4_array{vec4(1, 0, 0, 1), vec4(0, 1, 0, 1), vec4(0, 0, 1, 1)},
            vert = am.vec2_array{vec2(-1, -1), vec2(0, 1), vec2(1, -1)}
        }
        ^ am.draw("triangles")

Binding shader inputs
---------------------

Drawing primitives
------------------

Transforms
----------

Textures
--------

..  _framebuffers:

Framebuffers
------------

Graphics API reference
----------------------

..  function:: am.window(settings)

    Creates a new window, and returns the window object.
    ``settings`` is a table of window settings.

    **Available settings:**

    ``mode``
        Either ``"windowed"`` or ``"fullscreen"``.
        A fullscreen window will have the same resolution as the
        user's desktop.
        The default is ``"windowed"``.
        Not all platforms support windowed mode (e.g. iOS). On these
        platforms this setting is ignored.

    ``width`` ``height``
        The size of the window. These settings are ignored in
        fullscreen mode, or if windowed mode is not supported on
        the target platform.
        Think of these settings as hints. The window may end up being
        bigger or smaller. You can get the actual window size with the
        ``width`` and ``height`` fields after it is created.
        The default is 640x480.

    ``top`` ``left``
        The position of the top-left corner of the window.
        These are ignored for fullscreen windows.

    ``title``
        The window title.

    ``resizable``
        ``true`` or ``false``. This determines whether the window
        can be resized by the user. The default is ``true``.
        Ignored for fullscreen windows.

    ``borderless``
        ``true`` or ``false``. A borderless window has no title bar
        or border. The default is ``false``.

    ``depth_buffer``
        ``true`` or ``false``. This determines whether the window
        has a depth buffer. The default is ``false``.

    ``stencil_buffer``
        ``true`` or ``false``. This determines whether the window
        has a stencil buffer. The default is ``false``.

    ``lock_pointer``
        ``true`` or ``false``. 
        When pointer lock is enabled the cursor will be hidden and mouse
        movement will be set to "relative" mode. In this mode the mouse is
        tracked infinitely in all directions, i.e. as
        if there is no edge of the screen to stop the mouse cursor.
        This is useful for implementing first person style mouse-look.
        The default is ``false``.

    ``clear_color``
        The color used to clear the window each frame before drawing
        as a ``vec4``. The default clear color is black.

    ``msaa_samples``
        The number of samples to use for multisample anti-aliasing.
        This must be a power of 2. Use zero for no anti-aliasing.
        The default is zero.

    ``orientation``
        Either ``"portrait"`` or ``"landscape"``.
        This specifies the supported orientation of the
        window on platforms that support orientation changes (e.g. iOS)
        If omitted, both orientations are supported.


    **Window fields:**

    ..  object:: window.width

        The current window width in pixels.
        
        Readonly.

    ..  object:: window.height

        The current window height in pixels.
        
        Readonly.

    ..  object:: window.mode

        ``"fullscreen"`` or ``"windowed"``.
        
        Updatable.

    ..  object:: window.clear_color

        The color to use to clear the window each frame (a ``vec4``).
        
        Updatable.

    ..  object:: window.lock_pointer

        Determines whether the mouse pointer is locked to the window.
        If the pointer is locked it is hidden and movement is tracked
        in "relative" mode as if the screen has no edges.

        Updatable.

    ..  object:: window.scene

        The scene node currently attached to the window.
        This scene will be rendered to the window each frame.

        Updatable.


    **Window methods:**

    ..  function:: window:resized()

        Returns true if the window's size changed since the last frame.

    ..  function:: window:close()

        Closes the window and quits the application if this was
        the only window.

