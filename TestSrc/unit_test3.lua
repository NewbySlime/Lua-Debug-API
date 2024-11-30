
function delay_process(delay)
  -- place holder, in case of not set up
end


function read_output_file(file_name)
  output_file = io.open(file_name, "r")
  numidx = 1
  while true do
    str = output_file:read("l")
    if str == nil then
      break
    end

    print(string.format("%d. %s", numidx, str))
    numidx = numidx + 1
  end
  output_file:close()
end


function test_io_library(log_file_name)
  print("Opening log file...");
  os.execute(string.format("code -r %s", log_file_name))

  print("Testing IO library...")
  print("Testing opening non-existent file...")
  success = io.open("this_object_should_not_exists.txt", "r")
  if success ~= nil then
    print("ERROR: opening a non-existent file did not throw an error.")
    return
  end
  print("Success, error thrown.")

  print("Printing directly from IO...")
  io.output():write("this is a test from direct IO function.\n")
  print("Done.")
  
  print("Testing printing to 'test_default_output.txt' from IO...")
  io.output("TestSrc/test_default_output.txt")
  io.write("test default output data")
  print("Done.")

  print("Opening temporary file...")
  print("(This should be manually checked by the tester.)")
  tmpfileh = io.tmpfile()
  tmpfileh:write("this is a test string that would be deleted.")
  tmpfileh:close()
  print("File closed (should be removed).")

  print("Reading line per line in file 'TestSrc/test_lines.txt'...")
  numidx = 1
  for s in io.lines("TestSrc/test_lines.txt") do
    print(string.format("%d. %s", numidx, s))
    numidx = numidx + 1
  end
  print("Done.")

  print("'lines' function testing to open non-existent file...")
  success = pcall(io.lines, "test_lines.txt")
  if success == true then
    print("ERROR: opening a non-existent file did not throw an error.")
    return
  end
  print("Success, error thrown.")
  
  print("Checking the type of a file handle...")
  testfile_obj = io.open("TestSrc/test_file.txt", "w")
  success = io.type(testfile_obj)
  if success ~= "file" then
    print("ERROR: type does not correspond to a file handle.")
    return
  end
  print("Match, file handle does correspond to a type.")

  print("Test 'write' function, but the file already closed...")
  io.output(testfile_obj)
  testfile_obj:close()
  success = pcall(io.write, "test")
  if success == true then
    print("ERROR: writing to a file that is closed does not throw an error.")
    return
  end
  print("Success, error thrown.")

  print("Checking the type of previous file handle...")
  success = io.type(testfile_obj)
  if success ~= "closed file" then
    print("ERROR: type does not correspond to a closed file handle.")
    return
  end
  print("Match, closed file handle does correspond to a type.")

  _leakfile = io.open("testleak_file.txt")

  print("\nTesting file handle...")
  print("Writing to output file...")
  output_file = io.open("TestSrc/test_output.txt", "w")
  output_file:write("first line\nsecond line\nthird line\n")
  output_file:close()
  print("Done.")

  print("Reading from output file...")
  read_output_file("TestSrc/test_output.txt")
  print("Done.")

  print("Appending to output file...")
  output_file = io.open("TestSrc/test_output.txt", "a")
  output_file:write("fourth line\nfifth line\n")
  output_file:close()
  print("Done.")

  print("Reading the output file again...")
  read_output_file("TestSrc/test_output.txt")
  print("Done.")

  print("Testing r+ open mode...")
  print("Trying to write from the beginning of file...")
  output_file = io.open("TestSrc/test_output.txt", "r+")
  output_file:seek("set")
  output_file:write("this line should be overwritten")
  output_file:close()
  print("Done.")

  print("Reading the output file again...")
  read_output_file("TestSrc/test_output.txt")
  print("Done.")

  print("Testing a+ open mode...")
  print("Trying to write from the beginning of file (should not be overwritten)...")
  output_file = io.open("TestSrc/test_output.txt", "a+")
  output_file:seek("set")
  output_file:write("this is the end of line")
  output_file:close()
  print("Done.")

  print("Reading the output file again...")
  read_output_file("TestSrc/test_output.txt")
  print("Done.")

  print("Testing w+ open mode (file should be cleared)...")
  output_file = io.open("TestSrc/test_output.txt", "w+")
  output_file:write("file should be cleared")
  output_file:close()
  print("Done.")

  print("Reading the output file again...")
  read_output_file("TestSrc/test_output.txt")
  print("Done.")

  print("Testing seeking function...")
  print("Reading each character by 1 interval...")
  test_seek_file = io.open("TestSrc/test_seek.txt", "r")
  numidx = 1
  while true do
    str = test_seek_file:read(1)
    if str == nil then
      break
    end

    print(string.format("%d. %s", numidx, str))
    numidx = numidx + 1

    test_seek_file:seek("cur", 1)
  end
  print("Done.")

  test_seek_file:seek("set", 1)
  str = test_seek_file:read(1)
  print(string.format("Second character in file: %s", str))

  test_seek_file:seek("end", -1)
  str = test_seek_file:read(1)
  print(string.format("Last character in file: %s", str))

  test_seek_file:close()

  print("Testing changing the buffering mode...")
  test_buffer_file_input = io.open("TestSrc/test_buffer_input.txt", "r")
  test_buffer_file_output = io.open("TestSrc/test_buffer_output.txt", "w")
  test_buffer_file_output:write("test")
  test_buffer_file_output:close()

  print("Testing no buffering mode...")
  delay_process(1000)
  os.execute("code -r TestSrc/test_buffer_output.txt")
  test_buffer_file_output = io.open("TestSrc/test_buffer_output.txt", "w")
  test_buffer_file_output:setvbuf("no")
  test_buffer_file_input:seek("set")
  while true do
    str = test_buffer_file_input:read(1)
    if str == nil then
      break
    end

    test_buffer_file_output:write(str)
    delay_process(50)
  end

  test_buffer_file_output:close()
  delay_process(500)
  os.execute(string.format("code -r %s", log_file_name))
  print("Done.")

  print("Testing line buffering mode...")
  delay_process(1000)
  os.execute("code -r TestSrc/test_buffer_output.txt")
  test_buffer_file_output = io.open("TestSrc/test_buffer_output.txt", "w")
  test_buffer_file_output:setvbuf("line")
  test_buffer_file_input:seek("set")
  while true do
    str = test_buffer_file_input:read(1)
    if str == nil then
      break
    end

    test_buffer_file_output:write(str)
    delay_process(50)
  end

  test_buffer_file_output:close()
  delay_process(500)
  os.execute(string.format("code -r %s", log_file_name))
  print("Done.")

  print("Testing full buffering mode...")
  delay_process(1000)
  os.execute("code -r TestSrc/test_buffer_output.txt")
  test_buffer_file_output = io.open("TestSrc/test_buffer_output.txt", "w")
  test_buffer_file_output:setvbuf("full")
  test_buffer_file_input:seek("set")
  while true do
    str = test_buffer_file_input:read(1)
    if str == nil then
      break
    end

    test_buffer_file_output:write(str)
    delay_process(50)
  end

  test_buffer_file_output:close()
  delay_process(500)
  os.execute(string.format("code -r %s", log_file_name))
  print("Done.")

  delay_process(3000)
  test_buffer_file_input:close()

  print("Testing running a function of a closed file...")
  success = pcall(test_buffer_file_output.write, test_buffer_file_input, "test")
  if success == true then
    print("ERROR: using a closed file does not throw an error?")
    return
  end
  print("Success, error thrown.")
end