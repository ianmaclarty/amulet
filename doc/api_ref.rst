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

    ``width`` and ``height``
        The desired size of the window. This is not necessarily the
        size of the window in pixels. Instead it's the size of
        the window's coordinate space which is (``0``, ``0``) in the
        bottom-left corner and (``width``, ``height``) in the
        top-right corner if letterboxing is enabled (the default).
        If letterboxing
        is disabled, then the coordinate system will extend
        in the horizontal or vertical directions to
        ensure an area of at least ``width`` x ``height`` is
        visible in the center of the window.
        The bottom-left corner of this area will have coordinate
        (``0``, ``0``).

    ``title``
        The window title.

    ``resizable``
        ``true`` or ``false``. This determines whether the window
        can be resized by the user. The default is ``true``.

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

    ``letterbox``
        ``true`` or ``false``. Indicates whether the original
        aspect ratio (as determined by the ``width`` and ``height`` settings
        of the window) should be maintained after a resize by adding
        horizontal or vertical bars to the sides of the window.
        The bars will be the same color as the ``clear_color`` settings for the
        window.
        The default is ``true``.

    ``msaa_samples``
        The number of samples to use for multisample anti-aliasing.
        This must be a power of 2. Use zero for no anti-aliasing.
        The default is zero.

    ``orientation``
        Either ``"portrait"`` or ``"landscape"``.
        This specifies the supported orientation of the
        window on platforms that support orientation changes (e.g. iOS)
        If omitted, both orientations are supported.

    ``projection``
        A custom projection matrix (a ``mat4``) that overrides the
        window's default coordinate space.

    **Window fields:**

    ..  object:: window.left

        The x coordinate of the left edge of the 
        window in the window's default coordinate space. This will always be 0 if
        ``letterbox`` is enabled, but can be negative if ``letterbox``
        is disabled.

        Readonly.

    ..  object:: window.right

        The x coordinate of the right edge of the window, in the window's
        default coordinate space.

        Readonly.

    ..  object:: window.bottom

        The y coordinate of the bottom edge of the window, in the window's
        default coordinate space.  This will always be 0 if ``letterbox`` is
        enabled, but can be negative if ``letterbox`` is disabled.

        Readonly.

    ..  object:: window.top

        The y coordinate of the top edge of the window, in the window's
        default coordinate space.
    
        Readonly.

    ..  object:: window.width
        
        The width of the window in the window's default coordinate space.
        This will always be equal to the ``width`` setting supplied
        when the window was created if the ``letterbox`` setting is
        enabled. Otherwise it may be larger, but it will never be
        smaller than the ``width`` setting.
        
        Readonly.

    ..  object:: window.height

        The height of the window in the window's default coordinate space.
        This will always be equal to the ``height`` setting supplied
        when the window was created if the ``letterbox`` setting is
        enabled. Otherwise it may be larger, but it will never be
        smaller than the ``height`` setting.
        
        Readonly.

    ..  object:: window.pixel_width

        The width of the window in pixels.
        
        Readonly.

    ..  object:: window.pixel_height

        The height of the window in pixels
        
        Readonly.

    ..  object:: window.mode

        ``"fullscreen"`` or ``"windowed"``.
        
        Updatable.

    ..  object:: window.clear_color

        The color to use to clear the window each frame (a ``vec4``).
        
        Updatable.

    ..  object:: window.letterbox

        The window's letterbox setting (see above).
        
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

    ..  object:: window.projection

        A custom projection matrix (a ``mat4``). This overrides
        the windows's default coordinate space.
        Set it to nil to use the default coordinate space.

        Updatable.

    **Window methods:**

    ..  function:: window:resized()

        Returns true if the window's size changed since the last frame.

    ..  function:: window:close()

        Closes the window and quits the application if this was
        the only window.

    ..  function:: window:mouse_position()

        The position of the mouse cursor, as a ``vec2``,
        in the window's coordinate system.

    ..  function:: window:mouse_position_norm()

        The position of the mouse cursor in normalized device coordinates,
        as a ``vec2``.

    ..  function:: window:mouse_pixel_position()

        The position of the mouse cursor in pixels where the bottom left
        corner of the window has coordinate (0, 0), as a ``vec2``.

    ..  function:: window:mouse_position_norm()

        The position of the mouse cursor in normalized device coordinates,
        as a ``vec2``.

    ..  function:: window:mouse_delta()

        The change in mouse position since the last frame, in
        the window's coordinate system (a ``vec2``).

    ..  function:: window:mouse_delta_norm()

        The change in mouse position since the last frame, in
        normalized device coordinates (a ``vec2``).

    ..  function:: window:mouse_pixel_delta()

        The change in mouse position since the last frame, in
        pixels (a ``vec2``).

    ..  function:: window:mouse_down(button)

        Returns true if the given button was being pressed
        at the start of the frame. ``button`` may be ``"left"``,
        ``"right"`` or ``"middle"``.

    ..  function:: window:mouse_pressed(button)

        Returns the number of times the given mouse button was
        pressed since the last frame, except if the button was
        not pressed, in which case ``nil`` is returned.
        ``button`` may be ``"left"``, ``"right"`` or ``"middle"``.

    ..  function:: window:mouse_released(button)

        Returns the number of times the given mouse button was
        released since the last frame, except if the button was
        not released, in which case ``nil`` is returned.
        ``button`` may be ``"left"``, ``"right"`` or ``"middle"``.
