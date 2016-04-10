
# Controllers

### am.controller_lt_val(index) {#am.controller_lt_val .func-def}

Returns the value of the left trigger axis of controller `index`,
where `index` starts from 1. The returned value is between 0 and 1.

### am.controller_rt_val(index) {#am.controller_rt_val .func-def}

Returns the value of the right trigger axis of controller `index`,
where `index` starts from 1. The returned value is between 0 and 1.

### am.controller_lstick_pos(index) {#am.controller_lstick_pos .func-def}

Returns the position of the left stick of controller `index`
as a `vec2`, where `index` starts from 1.

### am.controller_rstick_pos(index) {#am.controller_rstick_pos .func-def}

Returns the position of the left stick of controller `index`
as a `vec2`, where `index` starts from 1.

### am.controller_button_pressed(index, button) {#am.controller_button_pressed .func-def}

Returns `true` if the given button of controller `index`
was pressed since the last frame, where `index` starts
from 1.

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
was released since the last frame, where `index` starts
from 1. See [am.controller_button_pressed](#am.controller_button_pressed)
for a list of valid values for `button`.

### am.controller_button_down(index, button) {#am.controller_button_down .func-def}

Returns `true` if the given button of controller `index`
was down at the start of the current frame, where `index` starts
from 1. See [am.controller_button_pressed](#am.controller_button_pressed)
for a list of valid values for `button`.
