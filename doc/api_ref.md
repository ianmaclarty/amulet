
API reference
=============

## Maths functions

### vec2(...) {.func-def}

Constructs a 2 dimensional vector. See vec-cons for more details.

### vec3(...) {.func-def}

Constructs a 3 dimensional vector. See vec-cons for more details.

### vec4(...) {.func-def}

Constructs a 4 dimensional vector. See vec-cons for more details.

### math.dot (vector1, vector2) {.func-def}

Returns the dot product of two vectors. The vectors must have the same
size.

### math.cross(vector1, vector2) {.func-def}

Returns the cross product of two 3 dimensional vectors.

### math.normalize(vector) {.func-def}

Returns the normalized form of a vector (i.e. the vector that points in
the same direction, but whose length is 1). If the given vector has zero
length, then a vector of the same size is returned whose first component
is 1 and whose remaining components are 0.

### math.length(vector) {.func-def}

Returns the length of a vector.

### math.distance(vector1, vector2) {.func-def}

Returns the distance between two vectors.

### mat2(...) {.func-def}

Constructs a 2x2 matrix. See mat-cons for more details.

### mat3(...) {.func-def}

Constructs a 3x3 matrix. See mat-cons for more details.

### mat4(...) {.func-def}

Constructs a 4x4 matrix. See mat-cons for more details.

### math.inverse(matrix) {.func-def}

Returns the inverse of a matrix.

### math.lookat(eye, center, up) {.func-def}

Creates a 4x4 view matrix at `eye`, looking in the direction of
`center` with the y axis of the camera pointing in the direction same
direction as `up`.

### math.perspective(fovy, aspect, near, far) {.func-def}

Creates a 4x4 matrix for a symetric perspective-view frustum.

-   `fovy` is the field of view in the y plain, in radians.
-   `aspect` is typically the window width divided by its height.
-   `near` and `far` are the distances of the near and far clipping
    plains from the camera (these should be positive).

### math.ortho(left, right, bottom, top \[, near, far\]) {.func-def}

Creates a 4x4 orthographic projection marix.

### quat(...) {.func-def}

Constructs a quaternion. See quat-cons for more details.

### math.perlin(pos \[, period\]) {.func-def}

Generate perlin noise. `pos` can be a 2, 3, or 4 dimensional vector,
or a number. If the second argument is supplied then the noise will be
periodic with the given period. `period` should be of the same type as
`pos` and its components should be integers greater than 1 (I'm not
sure exactly why, but using non-integer values doesn't seem to work
with the implementation of perlin noise Amulet currently uses).

The returned value is between -1 and 1.

### math.simplex(pos) {.func-def}

Generate simplex noise. `pos` can be a 2, 3, or 4 dimensional vector,
or a number.

The returned value is between -1 and 1.

### math.mix(from, top, t) {.func-def}

Returns the linear interpolation between `from` and `to` determined by
`t`. `from` and `to` can be numbers or vectors, and must be the same
type. `t` should be a number between 0 and 1. `from` and `to` can also
be quaternions. In that case `math.mix` returns the spherical linear
interpolation of the two quaternions.

## Window functions

### am.window(settings) {#window_func .func-def}

Creates a new window, returning a window object. `settings` is a
table with any of the following fields:

- **`mode`**:
    Either `"windowed"` or `"fullscreen"`. A fullscreen window will
    have the same resolution as the user's desktop. The default is
    `"windowed"`. Not all platforms support windowed mode (e.g. iOS).
    On these platforms this setting is ignored.

- **`width` and `height`**:
    The desired size of the window. This is not necessarily the size
    of the window in pixels (although it usually is if the window
    is created in `"windowed"` mode). Instead it defines the size of the window's
    *default coordinate system*. If letterboxing is enabled then this
    is (`-width/2`, `-height/2`) in the bottom-left corner and
    (`width/2`, `height/2`) in the top-right corner.
    If letterboxing is disabled, then the
    coordinate system will extend in the horizontal or vertical
    directions to ensure an area of at least `width`×`height` is
    visible in the center of the window. In either case the centre
    coordinate will always be (0, 0).
    The default size is 640×480.

- **`title`**:
    The window title.

- **`resizable`**:
    Whether the window can be resized by the user (`true` or `false`,
    default `true`).

- **`borderless`**:
    Whether the window has a title bar and border (`true` or `false`,
    default `false`).

- **`depth_buffer`**:
    Whether the window has a depth buffer (`true` or `false`,
    default `false`).

- **`stencil_buffer`**:
    Whether the window has a stencil buffer (`true` or `false`,
    default `false`).

- **`lock_pointer`**:
    `true` or `false`. When pointer lock is enabled the cursor will be
    hidden and mouse movement will be set to "relative" mode. In this
    mode the mouse is tracked infinitely in all directions, i.e. as if
    there is no edge of the screen to stop the mouse cursor. This is
    useful for implementing first-person style mouse-look. The default
    is `false`.

- **`clear_color`**:
    The color (a `vec4`) used to clear the window each frame before drawing.
    The default clear color is black (`vec4(0, 0, 0, 1)`).

- **`letterbox`**:
    `true` or `false`. Indicates whether the original aspect ratio (as
    determined by the `width` and `height` settings of the window)
    should be maintained after a resize by adding black horizontal or
    vertical bars to the sides of the window. The default is `true`.

- **`msaa_samples`**:
    The number of samples to use for multisample anti-aliasing. This
    must be a power of 2. Use zero (the default) for no anti-aliasing.

- **`orientation`**:
    `"portrait"` or `"landscape"`. This specifies the supported
    orientation of the window on platforms that support orientation
    changes (e.g. iOS). If omitted, both orientations are supported.

- **`projection`**:
    A custom projection matrix (a `mat4`) to be used for the window's
    coordinate system. This matrix is used when transforming
    mouse or touch event coordinates and is set as the projection
    matrix for rendering, but does not affect the `left`, `right`,
    `top`, `bottom`, `width` and `height` fields of the window.

## Window object fields

### window.left {.func-def}

The x coordinate of the left edge of the 
window in the window's default coordinate system.

Readonly.

### window.right {.func-def}

The x coordinate of the right edge of the window, in the window's
default coordinate system.

Readonly.

### window.bottom {.func-def}

The y coordinate of the bottom edge of the window, in the window's
default coordinate system.

Readonly.

### window.top {.func-def}

The y coordinate of the top edge of the window, in the window's
default coordinate system.

Readonly.

### window.width {.func-def}

The width of the window in the window's default coordinate system.
This will always be equal to the `width` setting supplied
when the window was created if the `letterbox` setting is
enabled. Otherwise it may be larger, but it will never be
smaller than the `width` setting.

Readonly.

### window.height {.func-def}

The height of the window in the window's default coordinate space.
This will always be equal to the `height` setting supplied
when the window was created if the `letterbox` setting is
enabled. Otherwise it may be larger, but it will never be
smaller than the `height` setting.

Readonly.

### window.pixel_width {.func-def}

The real width of the window in pixels.

Readonly.

### window.pixel_height {.func-def}

The real height of the window in pixels

Readonly.

### window.mode {.func-def}

See [window settings](#window_func).

Updatable.

### window.clear_color {.func-def}

See [window settings](#window_func).

Updatable.

### window.letterbox {.func-def}

See [window settings](#window_func).

Updatable.

### window.lock_pointer {.func-def}

See [window settings](#window_func).

Updatable.

### window.scene {.func-def}

The scene node currently attached to the window.
This scene will be rendered to the window each frame.

Updatable.

### window.projection {.func-def}

See [window settings](#window_func).

Updatable.

## Window object methods

### window:resized() {.func-def}

Returns true if the window's size changed since the last frame.

### window:close() {.func-def}

Closes the window and quits the application if this was
the only window.

### window:mouse_position() {.func-def}

Returns the position of the mouse cursor, as a `vec2`,
in the window's coordinate system.

### window:mouse_position_norm() {.func-def}

Returns the position of the mouse cursor in normalized device coordinates,
as a `vec2`.

### window:mouse_pixel_position() {.func-def}

Returns the position of the mouse cursor in pixels where the bottom left
corner of the window has coordinate (0, 0), as a `vec2`.

### window:mouse_position_norm() {.func-def}

Returns the position of the mouse cursor in normalized device coordinates,
as a `vec2`.

### window:mouse_delta() {.func-def}

Returns the change in mouse position since the last frame, in
the window's coordinate system (a `vec2`).

### window:mouse_delta_norm() {.func-def}

Returns the change in mouse position since the last frame, in
normalized device coordinates (a `vec2`).

### window:mouse_pixel_delta() {.func-def}

Returns the change in mouse position since the last frame, in
pixels (a `vec2`).

### window:mouse_down(button) {.func-def}

Returns true if the given button was being pressed
at the start of the frame. `button` may be `"left"`,
`"right"` or `"middle"`.

### window:mouse_pressed(button) {.func-def}

Returns the number of times the given mouse button was
pressed since the last frame, except if the button was
not pressed, in which case `nil` is returned.
`button` may be `"left"`, `"right"` or `"middle"`.

### window:mouse_released(button) {.func-def}

Returns the number of times the given mouse button was
released since the last frame, except if the button was
not released, in which case `nil` is returned.
`button` may be `"left"`, `"right"` or `"middle"`.

### window:mouse_wheel() {.func-def}

Returns the mouse scroll wheel position (a `vec2`).

### window:mouse_wheel_delta() {.func-def}

Returns the change in mouse scroll wheel position since the
last frame (a `vec2`).
