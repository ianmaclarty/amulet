local module3 = ...
local module2 = require'submodules.module2' -- cycle

print("loading module 3")

function module3.test()
    print("running module 3 test")
    print(module2.value)
end
