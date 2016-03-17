# Saving/loading state

### am.save_state(key, state [,format])

Save the table `state` under `key`. State 
may contain other tables, numbers, strings
or bools.

`format` can be `json` or `lua`. The default is
`lua`.

**Note**:
The `lua` format allows users to execute arbitrary lua
code my modifying the save file.

### am.load_state(key, [,format])

Loads the table saved under `key` and returns
it. If no table has been previously saved under `key`
then `nil` is returned.

`format` can be `json` or `lua`. The default is
`lua`.

**Note**:
The `lua` format allows users to execute arbitrary lua
code my modifying the save file.

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

# Preventing accidental global variable usage

### noglobals() {#noglobals .func-def}

Prevents the creation of new global variables.
An error will be raised if a new global is created after this
call, or if an attempt is made to read a nil global.

# Globbing

### am.glob(patterns) {#am.glob .func-def}

Returns an array (table) of file names matching the given glob pattern(s).
`patterns` should be a table of glob pattern strings.
A glob pattern is a file path with zero or more wildcard (`*`) characters.

Any matching files that are directories will have a slash (`/`)
appended to their names, even on Windows.

The slash character (`/`) can be used as a directory separator
on Windows (you don't need to use `\`).
Furthermore returned paths will always have '/' as the directory separator,
even on Windows.

**Note:**
This function only searches for files on the file system. It won't search
the resource archive in a exported game. Its intended use is
for writing file processing utilities and not for use directly in games
you wish to distribute.

Example:

~~~ {.lua}
local image_files = am.glob{"images/*.png", "images/*.jpg"}
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

# Platform

### am.platform {#am.platform .field-def}

The platform Amulet is running on. It will be one of the strings
`"linux"` `"windows"` `"osx"` `"ios"` `"android"` or `"html"`.

# Game Center (iOS only)

The following functions are only available on iOS.

### am.init_gamecenter() {#am.init_gamecenter .func-def}

Initialize Game Center. This must be called before any other
Game Center functions.

### am.gamecenter_available() {#am.gamecenter_available .func-def}

Returns true if Game Center was successfully initialized.

### am.submit_gamecenter_score(leaderboard_id, score) {#am.submit_gamecenter_score .func-def}

Submit a score to a leaderboard. Note that Game Center accepts only integer scores.

### am.submit_gamecenter_achievement(achievment_id) {#am.submit_gamecenter_achievement .func-def}

Submit an achievement.

### am.show_gamecenter_leaderboard(leaderboard_id) {#am.show_gamecenter_leaderboard .func-def}

Display a leaderboard.
