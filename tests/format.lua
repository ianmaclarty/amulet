function fmt(n)
    if type(n) == "number" then
        return string.format("%.16g", n)
    else
        return tostring(n)
    end
end

function printf(n)
    print(fmt(n))
end

