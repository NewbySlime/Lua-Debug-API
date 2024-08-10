print("[Lua] Hello world! From Lua.")

program_state = {}


function create_random_key()
  local _res = "";

  local _randlen = math.random(4, 8);
  for i = 1, _randlen do
    local _rand_char = string.char(math.random(1, 100))
    _res = _res .. _rand_char
  end

  return _res
end

function fill_program_state(i_iter)
  print("[Lua] Called fill_program_state")
  --[[
  program_state[0] = 1
  program_state[1] = 2
  program_state[2] = {
    1,2,3,4
  }
  --]]
  
  for i=0, 3, 1 do
    local _str_key = "k\0" .. create_random_key()
    print(string.format("str key %s", _str_key))
    program_state[_str_key] = create_random_key()
  end
end


function calculate_add2(i1, i2)
  print(string.format("[Lua] Calculating %s + %s", i1, i2));
  return i1 + i2
end

function call_c_function()
  print("[Lua] Calling predefined function...")
  local _num = _called_func(7.2, 355);
  print(string.format("[Lua] function result %d", _num))
end