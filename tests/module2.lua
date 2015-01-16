local mod1 = require("module1")
local mod3 = require("module3")
print("loading module 2")

return {
    test = function()
        print("running module 2 test")
        mod1.test()
        mod3.test()
    end
}
