
# Logging

### log(msg, ...) {#log .func-def}

Log a message to the console.

`msg` may contain format specifiers like the standard Lua `string.format`
function.

The logged messages are prefixed with the file name
and line number where `log` was called.

Example:

~~~ {.lua}
log("here")
log("num = %g, string = %s", 1, "two")
~~~

# Running JavaScript

### am.eval_js(js) {#am.eval_js .func-def}

Runs the given JavaScript string and returns the
result as a Lua value. JavaScript objects and arrays
are converted to Lua tables and other JavaScript
types are converted to the corresponding Lua types.
`undefined` is converted to `nil`.

This function only works when running in a browser.
On other platforms it has no effect and always returns `nil`.

# Converting to/from JSON

### am.to_json(value) {#am.to_json .func-def}

Converts the given Lua value to a JSON string and
returns it.

Cycles are not detected.

### am.parse_json(json) {#am.parse_json .func-def}

Converts the given JSON string to a Lua value
and returns it.

# Loading scripts

### am.load_script(filename) {#am.load_script .func-def}

Loads the Lua script in `filename` and returns
a function that, when called, will run the script.

# Performance stats

### am.perf_stats() {#am.perf_stats .func-def}

Returns a table with the following fields:

- `avg_fps`: frames per second averaged over the last 60 frames
- `min_fps`: the minimum frames per second over the last 60 frames

# Amulet version

### am.version {#am.version .field-def}

The current Amulet version, as a string. E.g. `"1.0.3"`.
