#ifndef LUACPP_ERROR_HANDLING_HEADER
#define LUACPP_ERROR_HANDLING_HEADER

#include "lua_includes.h"


namespace lua{
  class error_var;

  // Call a function in a protected enclosure, if the function throws a lua::error_var it will try to normalize the lua_State and CPP stack, then call lua_error().
  // WARN: lua::error_var object should use the same compilation code with this function user.
  template<typename T_ret, T_ret def_ret, typename T_func, typename... T_args> T_ret protected_callcpp(lua_State* state, T_func func, T_args... args){
    int _prev_stack = 0;

    try{
      return func(state, args...);
    }
    catch(error_var* err){
      int _stack_delta = lua_gettop(state) - _prev_stack;
      if(_stack_delta > 0)
        lua_pop(state, _stack_delta);

      err->push_to_stack(state);

      cpplua_delete_variant(err);
      lua_error(state);
    }

    return def_ret;
  }
}

#endif