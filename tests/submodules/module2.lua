local module2 = ...
local mod1 = require("module1")
local mod3 = require("submodules.module3")
print("loading module 2")

function module2.test()
    print("running module 2 test")
    mod1.test()
    mod3.test()
end

module2.value = "MOD2"
