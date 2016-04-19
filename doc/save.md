
# Saving and loading state

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

