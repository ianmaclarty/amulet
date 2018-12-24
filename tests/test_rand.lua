local
function printf(n)
    print(string.format("%0.14g", n))
end

local rand = am.rand(5321112)
printf(rand())
printf(rand())
printf(rand())
printf(rand(2))
printf(rand(10))
printf(rand(7))
printf(rand(4, 10))
printf(rand(-10, 0))
printf(rand(-10, 10))
printf(rand(-10, 10))

local t = {1, 2, 3, 4, 5, 5}
table.shuffle(t, rand)
print(table.tostring(t))
