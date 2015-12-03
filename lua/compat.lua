local v = tonumber(_VERSION:match"^Lua ([0-9%.]+)$")
if v > 5.1 then

    -- behaviour of coroutine.running differs between 5.1 and 5.2,
    -- make sure to use 5.1 semantics (we depend on this for am.wait to work)
    local old_running = coroutine.running
    coroutine.running = function()
        local thread, is_main = old_running()
        if is_main then
            return nil
        else
            return thread
        end
    end

    -- make unpack a global as in 5.1
    rawset(_G, "unpack", table.unpack)

    -- loadstring is missing from 5.2
    rawset(_G, "loadstring", load)
end
