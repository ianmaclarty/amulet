
# Windows and input

## Creating a window

### am.window(settings) {#am.window .func-def}

Creates a new window and returns a handle to it. `settings` is a
table with any of the following fields:

- **`mode`**:
    Either `"windowed"` or `"fullscreen"`. A fullscreen window will
    have the same resolution as the user's desktop. The default is
    `"windowed"`. Not all platforms support windowed mode (e.g. iOS).
    On these platforms this setting is ignored.

- **`width`** and **`height`**:
    These define the window's *default coordinate system*.
    If letterboxing is enabled then this is (`-width/2`, `-height/2`) in the
    bottom-left corner and (`width/2`, `height/2`) in the top-right corner.
    If letterboxing is disabled, then the coordinate system will extend in the
    horizontal or vertical directions to ensure an area of at least
    `width`×`height` is visible in the center of the window. In either case the
    centre coordinate will always be (0, 0).  The default size is 640×480.
    Mouse and touch positions as well as rendering will be in this coordinate
    system unless a custom projection matrix is defined (see below).
    These also define the physical size of the window in windowed mode, unless the
    `physical_size` property is given (see below).

- **`physical_size`**:
    The initial physical size of the window in windowed mode (a `vec2`). 
    This is in "screen units" which usually correspond to pixels, but not
    always. For example on a retina Mac display where `highdpi` is enabled 
    there are 2 pixels per screen unit. If this property is omitted then
    the `width` and `height` properties will be used for the physical window
    size. Note that this property has no effect on the coordinate system
    used by the window. Note also that this property only has an effect where
    the platform supports different sized windows. E.g. on iOS windows always
    fill the screen, so this property is ignored.

- **`title`**:
    The window title.

- **`resizable`**:
    Whether the window can be resized by the user (`true` or `false`,
    default `true`).

- **`borderless`**:
    Whether the window has a title bar and border (`true` or `false`,
    default `false`).

- **`highdpi`**:
    Whether to use high DPI resolution if available (`true` or `false`,
    default `false`).

- **`depth_buffer`**:
    Whether the window has a depth buffer (`true` or `false`,
    default `false`).

- **`stencil_buffer`**:
    Whether the window has a stencil buffer (`true` or `false`,
    default `false`).

- **`stencil_clear_value`**:
    The value to clear the stencil buffer with before drawing each
    frame (an integer between 0 and 255). The default is 0.

- **`lock_pointer`**:
    `true` or `false`. When pointer lock is enabled the cursor will be
    hidden and mouse movement will be set to "relative" mode. In this
    mode the mouse is tracked infinitely in all directions, i.e. as if
    there is no edge of the screen to stop the mouse cursor. This is
    useful for implementing first-person style mouse-look. The default
    is `false`.

- **`show_cursor`**:
    Whether to show the mouse cursor (`true` or `false`, default `true`).

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

- **`projection`**:
    A custom projection matrix (a `mat4`) to be used for the window's
    coordinate system. If supplied, this matrix is used when transforming
    mouse or touch event coordinates and is set as the projection
    matrix for rendering, but does not affect the `left`, `right`,
    `top`, `bottom`, `width` and `height` fields of the window.

## Window fields

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

### window.height {.field-def}

The height of the window in the window's default coordinate space.
This will always be equal to the `height` setting supplied
when the window was created if the `letterbox` setting is
enabled. Otherwise it may be larger, but it will never be
smaller than the `height` setting.

Readonly.

### window.pixel_width {.field-def}

The real width of the window in pixels.

Readonly.

### window.pixel_height {.field-def}

The real height of the window in pixels

Readonly.

### window.safe_left {.func-def}

The x coordinate of the left edge of the 
window's safe area in the window's default coordinate system.

This should be used to position elements that you don't want obscured
by e.g. the iPhone X notch.

Readonly.

### window.safe_right {.func-def}

The x coordinate of the right edge of the window's safe area, in the window's
default coordinate system.

This should be used to position elements that you don't want obscured
by e.g. the iPhone X notch.

Readonly.

### window.safe_bottom {.func-def}

The y coordinate of the bottom edge of the window's safe area, in the window's
default coordinate system.

This should be used to position elements that you don't want obscured
by e.g. the iPhone X notch.

Readonly.

### window.safe_top {.func-def}

The y coordinate of the top edge of the window's safe area, in the window's
default coordinate system.

This should be used to position elements that you don't want obscured
by e.g. the iPhone X notch.

Readonly.

### window.mode {.field-def}

See [window settings](#am.window).

Updatable.

### window.clear_color {.field-def}

See [window settings](#am.window).

Updatable.

### window.stencil_clear_value {.field-def}

See [window settings](#am.window).

Updatable.

### window.letterbox {.field-def}

See [window settings](#am.window).

Updatable.

### window.lock_pointer {.field-def}

See [window settings](#am.window).

Updatable.

### window.show_cursor {.field-def}

See [window settings](#am.window).

Updatable.

### window.scene {#window.scene .field-def}

The [scene node](#scenes) currently attached to the window.
This scene will be rendered to the window each frame.

Updatable.

### window.projection {.field-def}

See [window settings](#am.window).

Updatable.

## Closing a window

### window:close() {.method-def}

Closes the window and quits the application if this was
the only window.

## Detecting when a window resizes

### window:resized() {.method-def}

Returns true if the window's size changed since the last frame.

## Detecting key presses

The following functions detect physical key states and are
not affected by the keyboard layout preferences selected
in the OS (except when targetting HTML).

Keys are represented by one of the following strings:

- `"a"`
- `"b"`
- `"c"`
- `"d"`
- `"e"`
- `"f"`
- `"g"`
- `"h"`
- `"i"`
- `"j"`
- `"k"`
- `"l"`
- `"m"`
- `"n"`
- `"o"`
- `"p"`
- `"q"`
- `"r"`
- `"s"`
- `"t"`
- `"u"`
- `"v"`
- `"w"`
- `"x"`
- `"y"`
- `"z"`
- `"1"`
- `"2"`
- `"3"`
- `"4"`
- `"5"`
- `"6"`
- `"7"`
- `"8"`
- `"9"`
- `"0"`
- `"enter"`
- `"escape"`
- `"backspace"`
- `"tab"`
- `"space"`
- `"minus"`
- `"equals"`
- `"leftbracket"`
- `"rightbracket"`
- `"backslash"`
- `"semicolon"`
- `"quote"`
- `"backquote"`
- `"comma"`
- `"period"`
- `"slash"`
- `"capslock"`
- `"f1"`
- `"f2"`
- `"f3"`
- `"f4"`
- `"f5"`
- `"f6"`
- `"f7"`
- `"f8"`
- `"f9"`
- `"f10"`
- `"f11"`
- `"f12"`
- `"printscreen"`
- `"scrolllock"`
- `"pause"`
- `"insert"`
- `"home"`
- `"pageup"`
- `"delete"`
- `"end"`
- `"pagedown"`
- `"right"`
- `"left"`
- `"down"`
- `"up"`
- `"numlock"`
- `"kp_divide"`
- `"kp_multiply"`
- `"kp_minus"`
- `"kp_plus"`
- `"kp_enter"`
- `"kp_1"`
- `"kp_2"`
- `"kp_3"`
- `"kp_4"`
- `"kp_5"`
- `"kp_6"`
- `"kp_7"`
- `"kp_8"`
- `"kp_9"`
- `"kp_0"`
- `"kp_period"`
- `"lctrl"`
- `"lshift"`
- `"lalt"`
- `"lgui"`
- `"rctrl"`
- `"rshift"`
- `"ralt"`
- `"rgui"`

Keys not listed above are represented as a hash followed by the scancode,
for example `"#101"`.

### window:key_down(key) {#key_down .method-def}

Returns true if the given key was down at the start of the
current frame.

### window:keys_down() {.method-def}

Returns an array of the keys that were down at the
start of the current frame.

### window:key_pressed(key) {.method-def}

Returns true if the given key's state changed from up
to down since the last frame.

Note that if `key_pressed` returns true for a particular key, then
`key_down` will also return `true`. Also if `key_pressed`
returns `true` for a particular key then `key_released` will return `false`
for the same key.
(If necessary, Amulet will postpone key release events to the
next frame to ensure this.)

### window:keys_pressed() {.method-def}

Returns an array of all the keys whose state changed from
up to down since the last frame.

### window:key_released(key) {.method-def}

Returns true if the given key's state changed from down
to up since the last frame.

Note that if `key_released` returns true for a particular key, then
`key_down` will return `false`. Also if `key_released`
returns `true` for a particular key then `key_pressed` will return `false`.
(If necessary, Amulet will postpone key press events to the
next frame to ensure this.)

### window:keys_released() {.method-def}

Returns an array of all the keys whose state changed from
down to up since the last frame.

## Detecting mouse events

### window:mouse_position() {.method-def}

Returns the position of the mouse cursor, as a `vec2`,
in the window's coordinate system.

### window:mouse_norm_position() {.method-def}

Returns the position of the mouse cursor in normalized device coordinates,
as a `vec2`.

### window:mouse_pixel_position() {.method-def}

Returns the position of the mouse cursor in pixels where the bottom left
corner of the window has coordinate (0, 0), as a `vec2`.

### window:mouse_delta() {.func-def}

Returns the change in mouse position since the last frame, in
the window's coordinate system (a `vec2`).

### window:mouse_norm_delta() {.method-def}

Returns the change in mouse position since the last frame, in
normalized device coordinates (a `vec2`).

### window:mouse_pixel_delta() {.method-def}

Returns the change in mouse position since the last frame, in
pixels (a `vec2`).

### window:mouse_down(button) {.method-def}

Returns `true` if the given button was down
at the start of the frame. `button` may be `"left"`,
`"right"` or `"middle"`.

### window:mouse_pressed(button) {.method-def}

Returns `true` if the given mouse button's state changed
from up to down since the
last frame.
`button` may be `"left"`, `"right"` or `"middle"`.

Note that if `mouse_pressed` returns `true` for a particular button then
`mouse_down` will also return `true`. Also 
if `mouse_pressed` returns `true` for a particular button then `mouse_released`
will return `false`. (If necessary, Amulet will
postpone button release events to the next frame to ensure this.)

### window:mouse_released(button) {.method-def}

Returns true if the given mouse button's state changed
from down to up since the last frame.
`button` may be `"left"`, `"right"` or `"middle"`.

Note that if `mouse_released` returns `true` for a particular button then
`mouse_down` will return `false`. Also 
if `mouse_released` returns `true` for a particular button then `mouse_pressed`
will return `false`. (If necessary, Amulet will
postpone button press events to the next frame to ensure this.)

### window:mouse_wheel() {.method-def}

Returns the mouse scroll wheel position (a `vec2`).

### window:mouse_wheel_delta() {.method-def}

Returns the change in mouse scroll wheel position since the
last frame (a `vec2`).

## Detecting touch events

### window:touches_began() {.method-def #touches_began}

Returns an array of the touches that began since the last frame.

Each touch is an integer and each time a new touch
occurs the lowest available integer greater than or equal to 1 is assigned to
the touch.

If there are no other active touches then the next touch
will always be 1, so if your interface only expects a single
touch at a time, you can just use 1 for all touch functions
that take a touch argument and any additional touches will be
ignored.

### window:touches_ended() {.method-def}

Returns an array of the touches that ended since the last frame.

See [`window:touches_began`](#touches_began) for more info about
the returned touches.

### window:active_touches() {.method-def}

Returns an array of the currently active touches.

See [`window:touches_began`](#touches_began) for more info about
the returned touches.

### window:touch_began([touch]) {.method-def}

Returns true if the specific touch began since the last frame.

The default value for `touch` is `1`.

See [`window:touches_began`](#touches_began) for additional
notes on the `touch` argument.

### window:touch_ended([touch]) {.method-def}

Returns true if the specific touch ended since the last frame.

The default value for `touch` is `1`.

See [`window:touches_began`](#touches_began) for additional
notes on the `touch` argument.

### window:touch_active([touch]) {#window:touch_active .method-def}

Returns true if the specific touch is active.

The default value for `touch` is `1`.

See [`window:touches_began`](#touches_began) for additional
notes on the `touch` argument.

### window:touch_position([touch]) {.method-def}

Returns the last touch position in the window's coordinate system
(a `vec2`).

The default value for `touch` is `1`.

See [`window:touches_began`](#touches_began) for additional
notes on the `touch` argument.

### window:touch_norm_position([touch]) {.method-def}

Returns the last touch position in normalized device coordinates
(a `vec2`).

The default value for `touch` is `1`.

See [`window:touches_began`](#touches_began) for additional
notes on the `touch` argument.

### window:touch_pixel_position([touch]) {.method-def}

Returns the last touch position in pixels, where the bottom left
corner of the window is (0, 0) (a `vec2`).

The default value for `touch` is `1`.

See [`window:touches_began`](#touches_began) for additional
notes on the `touch` argument.

### window:touch_delta([touch]) {.method-def}

Returns the change in touch position since the
last frame in the window's coordinate system (a `vec2`).

The default value for `touch` is `1`.

See [`window:touches_began`](#touches_began) for additional
notes on the `touch` argument.

### window:touch_norm_delta([touch]) {.method-def}

Returns the change in touch position since the
last frame in normalized device coordinates (a `vec2`).

The default value for `touch` is `1`.

See [`window:touches_began`](#touches_began) for additional
notes on the `touch` argument.

### window:touch_pixel_delta([touch]) {.method-def}

Returns the change in touch position since the
last frame in pixels (a `vec2`).

The default value for `touch` is `1`.

See [`window:touches_began`](#touches_began) for additional
notes on the `touch` argument.

### window:touch_force([touch]) {#window:touch_force .method-def}

Returns the force of the touch where 0 means no force and
1 is "average" force. Harder presses will result in values
larger than 1.

The default value for `touch` is `1`.

See [`window:touches_began`](#touches_began) for additional
notes on the `touch` argument.

### window:touch_force_available() {#window:touch_force_available .method-def}

Returns true if touch force is supported on the device.
