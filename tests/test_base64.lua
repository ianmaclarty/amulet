local
function test_bytes(bytes)
    local n = #bytes
    local buf1 = am.buffer(n)
    local view1 = buf1:view("ubyte", 0, 1)
    for i, b in ipairs(bytes) do
        view1[i] = b
    end
    local str1 = am.base64_encode(buf1)
    local buf2 = am.base64_decode(str1)
    local view2 = buf2:view("ubyte", 0, 1)
    print(str1)
    for i, b in ipairs(bytes) do
        print(view2[i])
        assert(view2[i] == b)
    end
    print("")
end

test_bytes{49}
test_bytes{1, 2}
test_bytes{123, 244, 255}
test_bytes{0}
test_bytes{0, 0}
test_bytes{0, 0, 0}
test_bytes{1, 2, 3, 4}
test_bytes{10, 20, 30, 40, 50, 60, 70, 80, 90, 100}
