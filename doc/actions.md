
# Actions

This section covers useful action utility functions.
For information on the action mechanism see the
description of the [`action`](#node:action) method.

## Delay action

### am.delay(seconds) {#am.delay .func-def}

Returns an action that does nothing for the given number
of seconds.

## Combining actions

### am.series(actions) {#am.series .func-def}

Returns an action that runs the given array
of actions one after another.

### am.parallel(actions) {#am.parallel .func-def}

Returns an action that runs the given array of
actions all at the same time. The returned action
finishes once all the actions in the array are
finished.

### am.loop(func) {#am.loop .func-def}

`func` should be a function that returns an
action. `am.loop` returns an action that repeatedly
runs the action returned by `func`.

## Tweens

### am.tween([target,] time, values [, ease]) {#am.tween .func-def}

Returns an action that changes the values of 
one or more fields of `target` to new values over
a given time period.

`time` is in seconds.

`values` is a table mapping field names to their
new values.

`ease` is an easing function. This function takes
a value between 0 and 1 and returns a value between
0 and 1. This determines the "shape" of the interpolation
between the two values. If omitted a linear interpolation
is used.

The following easing functions are pre-defined (though of course you can
define your own):

- `am.ease.linear`
- `am.ease.quadratic`
- `am.ease.cubic`
- `am.ease.hyperbola`
- `am.ease.sine`
- `am.ease.windup`
- `am.ease.elastic`
- `am.ease.bounce`
- `am.ease.cubic_bezier(x1, y1, x2, y2)`: This returns a cubic
  bezier ease function with the given control points.
- `am.ease.out(f)`: This takes an existing ease function and
  returns its reverse. E.g. if the existing ease function is
  slow and then fast, the new one will be fast and then slow.
- `am.ease.inout(f, g)`. This returns an ease function that
  uses the ease function `f` for the first half of the transition
  and `out(g)` for the second half.

Tweening works with fields that are
numbers or vectors (`vec2`, `vec3` or `vec4`).
It also works with [quaternions](#quaternions).
In all cases the [`math.mix`](#math.mix) function
is used to interpolate between the initial value and the
final value.

If target is omitted it is taken to be the node to which the
tween action is attached.

Example:

~~~ {.lua}
am.window{}.scene =
    am.rect(-100, -100, 100, 0, vec4(1, 0, 0, 1))
    :action(
        am.tween(1, {
            color = vec4(0, 1, 0, 1),
            y2 = 100
        }))
~~~

## Coroutine actions

An action can also be a coroutine. Inside a coroutine action
it can sometimes be useful to wait for another action to finish,
such as a tween. That is what the `am.wait` function is for:

### am.wait(action) {#am.wait .func-def}

Waits for the given action to finish before continuing.
This can only be called from within a coroutine.

Example:

~~~ {.lua}
am.window{}.scene =
    am.rect(-100, -100, 100, 0, vec4(1, 0, 0, 1))
    :action(coroutine.create(function(node)
        while true do
            am.wait(am.tween(node, 1, {
                color = vec4(0, 1, 0, 1),
                y2 = 100
            }))
            am.wait(am.tween(node, 1, {
                color = vec4(1, 0, 0, 1),
                y2 = 0
            }))
        end
    end))
~~~
