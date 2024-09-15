local f = io.open("test.txt", "a+")

f:write("this is another test")
f:seek("set", 0)
print(f:read("a"))
f:seek("set", 0)
f:write("this should overwrite test")
f:close()