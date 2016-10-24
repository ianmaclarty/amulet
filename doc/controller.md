
# Controllers

Each controller is assigned an index starting from 1. The first controller
attached will always have index 1, so if your game only uses one controller
you can just use index 1.

Controllers are supported on Windows, OSX and Linux and up to 8 controllers
can be connected at once.

### am.controller_present(index) {#am.controller_present .func-def}

Returns `true` if controller `index` is currently connected.

### am.controller_attached(index) {#am.controller_attached .func-def}

Returns `true` if controller `index` was attached since the last frame.

### am.controller_detached(index) {#am.controller_detached .func-def}

Returns `true` if controller `index` was removed since the last frame.

### am.controllers_present() {#am.controllers_present .func-def}

Returns a list of currently connected controller indexes.

### am.controllers_attached() {#am.controllers_attached .func-def}

Returns a list of controller indexes attached since the last frame.

### am.controllers_detached() {#am.controllers_detached .func-def}

Returns a list of controller indexes removed since the last frame.

### am.controller_lt_val(index) {#am.controller_lt_val .func-def}

Returns the value of the left trigger axis of controller `index`.
The returned value is between 0 and 1.

### am.controller_rt_val(index) {#am.controller_rt_val .func-def}

Returns the value of the right trigger axis of controller `index`.
The returned value is between 0 and 1.

### am.controller_lstick_pos(index) {#am.controller_lstick_pos .func-def}

Returns the position of the left stick of controller `index`
as a `vec2`.
The position values range
from -1 to 1 in both the x and y components. Negative x means left
and negative y means down.

### am.controller_rstick_pos(index) {#am.controller_rstick_pos .func-def}

Returns the position of the left stick of controller `index`
as a `vec2`.
The position values range
from -1 to 1 in both the x and y components. Negative x means left
and negative y means down.

### am.controller_button_pressed(index, button) {#am.controller_button_pressed .func-def}

Returns `true` if the given button of controller `index`
was pressed since the last frame.

`button` can be one of the following strings:

- `"a"`
- `"b"`
- `"x"`
- `"y"`
- `"back"`
- `"guide"`
- `"start"`
- `"ls"`
- `"rs"`
- `"lb"`
- `"rb"`
- `"up"`
- `"down"`
- `"left"`
- `"right"`

### am.controller_button_released(index, button) {#am.controller_button_released .func-def}

Returns `true` if the given button of controller `index`
was released since the last frame.
See [am.controller_button_pressed](#am.controller_button_pressed)
for a list of valid values for `button`.

### am.controller_button_down(index, button) {#am.controller_button_down .func-def}

Returns `true` if the given button of controller `index`
was down at the start of the current frame.
See [am.controller_button_pressed](#am.controller_button_pressed)
for a list of valid values for `button`.
