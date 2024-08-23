

local function sleep(n)
  os.execute("ping -n " .. tonumber(n+1) .. " localhost > NUL")
end

function test_func()
  print("this is from lua")

  sleep(10);
  print("this is another lua print")
end