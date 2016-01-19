
# Saving and loading state

## Saving

### am.save_state(name, state) {#am.save_state .func-def}

Saves `state` under `name`. `state` should be a Lua table
without any cyclic references.

How and where the data is saved is platform dependent.

## Loading

### am.load_state(name) {#am.load_state .func-def}

Loads a previously saved state and returns it.
If the given name was not found `nil` is returned.
