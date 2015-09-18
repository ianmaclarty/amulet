.. _api-ref:

API reference
=============

Maths
-----

Vector functions
~~~~~~~~~~~~~~~~

..  function:: vec2(...)

    Constructs a 2 dimensional vector. See :ref:`vec-cons` for more details.

..  function:: vec3(...)

    Constructs a 3 dimensional vector. See :ref:`vec-cons` for more details.

..  function:: vec4(...)

    Constructs a 4 dimensional vector. See :ref:`vec-cons` for more details.

..  function:: math.dot (vector1, vector2)

    Returns the dot product of two vectors. The vectors must have the same
    size.

..  function:: math.cross(vector1, vector2)

    Returns the cross product of two 3 dimensional vectors.

..  function:: math.normalize(vector)

    Returns the normalized form of a vector (i.e. the vector that points
    in the same direction, but whose length is 1). If the given vector has
    zero length, then a vector of the same size is returned whose first
    component is 1 and whose remaining components are 0.

..  function:: math.length(vector)

    Returns the length of a vector.

..  function:: math.distance(vector1, vector2)

    Returns the distance between two vectors.

Matrix functions
~~~~~~~~~~~~~~~~

..  function:: mat2(...)

    Constructs a 2x2 matrix. See :ref:`mat-cons` for more details.

..  function:: mat3(...)

    Constructs a 3x3 matrix. See :ref:`mat-cons` for more details.

..  function:: mat4(...)

    Constructs a 4x4 matrix. See :ref:`mat-cons` for more details.

..  function:: math.inverse(matrix)

    Returns the inverse of a matrix.

..  function:: math.lookat(eye, center, up)

    Creates a 4x4 view matrix at ``eye``, looking in the direction of
    ``center`` with the y axis of the camera pointing in the direction same
    direction as ``up``.

..  function:: math.perspective(fovy, aspect, near, far)

    Creates a 4x4 matrix for a symetric perspective-view frustum.

    -  ``fovy`` is the field of view in the y plain, in radians.
    -  ``aspect`` is typically the window width divided by its height.
    -  ``near`` and ``far`` are the distances of the near and far clipping plains from the camera (these should be positive).

..  function:: math.ortho(left, right, bottom, top [, near, far])

    Creates a 4x4 orthographic projection marix.

Quaternion functions
~~~~~~~~~~~~~~~~~~~~

..  function:: quat(...)

    Constructs a quaternion. See :ref:`quat-cons` for more details.

Noise functions
~~~~~~~~~~~~~~~

..  function:: math.perlin(pos [, period])

    Generate perlin noise. ``pos`` can be a 2, 3, or 4 dimensional vector, or a number.
    If the second argument is supplied then the noise will be periodic with the given
    period. ``period`` should be of the same type as ``pos`` and its components should
    be integers greater than 1 (I'm not sure exactly why, but using non-integer
    values doesn't seem to work with the implementation of perlin noise Amulet currently
    uses).

    The returned value is between -1 and 1.

..  function:: math.simplex(pos)

    Generate simplex noise. ``pos`` can be a 2, 3, or 4 dimensional vector, or a number.

    The returned value is between -1 and 1.

Interpolation functions
~~~~~~~~~~~~~~~~~~~~~~~

..  function:: math.mix(from, top, t)

    Returns the linear interpolation between ``from`` and ``to`` determined by ``t``.
    ``from`` and ``to`` can be numbers or vectors, and must be the same
    type. ``t`` should be a number between 0 and 1.
    ``from`` and ``to`` can also be quaternions. In that case ``math.mix``
    returns the spherical linear interpolation of the two quaternions.

Video
-----

Window functions
~~~~~~~~~~~~~~~~

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

