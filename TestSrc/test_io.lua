local f = io.open("test.txt", "a+")

f:seek("set")
f:write("hello!")
print(f:read(6))
f:close()