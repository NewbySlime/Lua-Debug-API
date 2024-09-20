local f = io.open("test.txt", "a+")

print(f:read(5))
f:close()