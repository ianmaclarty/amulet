local filename = "tests/音乐.out"
local f = io.open(filename, "w")
f:write("hello")
f:close()
for line in io.lines(filename) do
    print(line)
end
