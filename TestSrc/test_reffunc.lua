test_func()

function test_lua_func()
  print("test from lua")
end

local _tlua = check_func(test_lua_func)
check_func(_tlua)

local _tc = check_func(test_func)
_tc = check_func(_tc)
_tc = check_func(_tc)
_tc = check_func(_tc)
_tc = check_func(_tc)