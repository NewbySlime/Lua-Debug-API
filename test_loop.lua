

local function sleep(n)
  os.execute("ping -n " .. tonumber(n+1) .. " localhost > NUL")
end


local function nested6(n, m)
  return m-n
end

local function nested5(n, m)
  print("nested5")
  return nested6(m-n, n)
end

local function nested4(n, m)
  print("nested4")
  return nested5(m-n, n)
end

local function nested3(n, m)
  print("nested3")
  return nested4(m-n, n)
end

local function nested2(n, m)
  print("nested2")
  return nested3(m-n, n)
end

local function nested1(n, m)
  print("nested1")
  return nested2(n+m, n)
end


function test_loop(n)
  for i = 1, 10, 1 do
    print("i: " .. i)
  
    local test = nested1(i, i)
    print("t")
    print(test)
  end

end


-- test_loop()