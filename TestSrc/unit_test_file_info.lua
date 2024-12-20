function test_func()
  print()
  function another_func()

    print()
    function another_func()
      print()

    end
  end
  -- this is a comment

  --[[
    this is a multiline comment
    ]]

  print()
end


local test_var = 1
local function print_iter(n)
  for i = 1, n, 1 do
    test_print(string.format("number: %d", i));
  end
end