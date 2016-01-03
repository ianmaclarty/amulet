#!/usr/bin/env amulet
for _, module in ipairs{"_G", "am", "math", "coroutine", "string", "table", "os", "io"} do
    for name, _ in pairs(_G[module]) do
        if not name:match("^_") then
            print((module == "_G" and "" or module..".")..name)
        end
    end
end
