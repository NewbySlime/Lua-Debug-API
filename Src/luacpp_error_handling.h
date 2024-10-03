#ifndef LUACPP_ERROR_HANDLING_HEADER
#define LUACPP_ERROR_HANDLING_HEADER

#include "luaapi_compilation_context.h"
#include "luaapi_core.h"
#include "luaapi_stack.h"
#include "luaapi_util.h"
#include "luaincludes.h"


namespace lua{
  class error_var;

#define PROTECTED_CALLCPP_CORE_CLOSURE_BEGIN \
    int _prev_stack = 0; \
     \
    try{ \

#define PROTECTED_CALLCPP_CORE_CLOSURE_END \
    } \
    catch(error_var* err){ \
      int _stack_delta = lua_core->context->api_stack->gettop(lua_core->istate) - _prev_stack; \
      if(_stack_delta > 0) \
        lua_core->context->api_stack->pop(lua_core->istate, _stack_delta); \
       \
      err->push_to_stack(lua_core); \
       \
      /* Use this compilation specific deletion function. As template function will compiled the same as the caller. */ \
      cpplua_delete_variant(err); \
      lua_core->context->api_util->error(lua_core->istate); \
    }


  // Call a function in a protected enclosure, if the function throws a lua::error_var it will try to normalize the lua_State and CPP stack, then call lua_error().
  // WARN: lua::error_var object should use the same compilation code with this function user.
  template<typename T_ret, T_ret def_ret, typename T_func, typename... T_args> T_ret protected_callcpp_core(const lua::api::core* lua_core, T_func func, T_args... args){
    PROTECTED_CALLCPP_CORE_CLOSURE_BEGIN
    return func(args...);
    PROTECTED_CALLCPP_CORE_CLOSURE_END
    return def_ret;
  }

  template<typename T_func, typename... T_args> void protected_callcpp_core_void(const lua::api::core* lua_core, T_func func, T_args... args){
    PROTECTED_CALLCPP_CORE_CLOSURE_BEGIN
    func(args...);
    PROTECTED_CALLCPP_CORE_CLOSURE_END
  }


#ifdef LUA_CODE_EXISTS

  template<typename T_ret, T_ret def_ret, typename T_func, typename... T_args> T_ret protected_callcpp(lua_State* state, T_func func, T_args... args){
    lua::api::core _lc;
      _lc.istate = state;
      _lc.context = cpplua_get_api_compilation_context();

    return protected_callcpp_core<T_ret, def_ret, T_func, T_args...>(&_lc, func, args...);
  }

  template<typename T_func, typename... T_args> void protected_callcpp_void(lua_State* state, T_func func, T_args... args){
    lua::api::core _lc;
      _lc.istate = state;
      _lc.context = cpplua_get_api_compilation_context();
    
    protected_callcpp_core_void<T_func, T_args...>(&_lc, func, args...);
  }

#endif // LUA_CODE_EXISTS

}

#endif