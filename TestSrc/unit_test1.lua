# contains test code


function infinite_loop_test()
  while true do
    print("infinite loop");
  end
end


function test_table_data(table_data)
  print("test_table_data")
  print(table_data)

  table_data:set_data("test_key", "this is a test data")
  print(table_data:get_data("test_key"))
end

function set_table_data(table_data, key, value)
  print("set_table_data")
  print(table_data)
  print(key)
  print(value)
  table_data:set_data(key, value)
end