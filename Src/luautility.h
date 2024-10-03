#ifndef LUAUTIL_HEADER
#define LUAUTIL_HEADER

#include "luaincludes.h"
#include "luaapi_compilation_context.h"
#include "luaapi_core.h"
#include "luaapi_stack.h"
#include "luaapi_value.h"


namespace lua::utility{
  bool verify_lua_version(const lua::api::core* lc1, const lua::api::core* lc2);
  bool verify_lua_version(const lua::api::compilation_context* c1, const lua::api::compilation_context* c2);

  bool check_same_runtime(const lua::api::core* lc1, const lua::api::core* lc2);
  void* get_main_thread(const lua::api::core* lc);

#ifdef LUA_CODE_EXISTS
  // Compare with this compilation specific Lua's version.
  bool verify_lua_version(const lua::api::core* lc2);
  // Compare with this compilation specific Lua's version.
  bool verify_lua_version(const lua::api::compilation_context* c2);

  lua_State* get_main_thread(lua_State* state);

  lua::api::core as_lua_api_core(lua_State* state);
#endif


#define PSTACK_CALL_CLOSURE_BEGIN \
    int _prev_stack = api_s->gettop(istate);

#define PSTACK_CALL_CLOSURE_END \
    int _current_stack = api_s->gettop(istate); \
    int _delta_stack = _current_stack - _prev_stack; \
    if(_delta_stack > expected_offset_max) \
      api_s->pop(istate, _delta_stack - expected_offset_max); \
    else if(_delta_stack < expected_offset_min){ \
      for(int i = _delta_stack; i < expected_offset_min; i++) \
        api_v->pushnil(istate); \
    }

  template<typename T_result, typename T_func, typename... T_args> T_result pstack_call(
    void* istate,
    int expected_offset_max,
    int expected_offset_min,
    lua::api::I_stack* api_s,
    lua::api::I_value* api_v,
    T_func fn,
    T_args... args
  ){
    PSTACK_CALL_CLOSURE_BEGIN
    T_result _res = fn(args...);
    PSTACK_CALL_CLOSURE_END
    return _res;
  }

  template<typename T_func, typename... T_args> void pstack_call_void(
    void* istate,
    int expected_offset_max,
    int expected_offset_min,
    lua::api::I_stack* api_s,
    lua::api::I_value* api_v,
    T_func fn,
    T_args... args
  ){
    PSTACK_CALL_CLOSURE_BEGIN
    fn(args...);
    PSTACK_CALL_CLOSURE_END
  }

  
  template<typename T_result, typename T_func, typename... T_args> T_result pstack_call_context(
    void* istate,
    int expected_offset_max,
    int expected_offset_min,
    const lua::api::compilation_context* context,
    T_func fn,
    T_args... args
  ){
    return pstack_call<T_result, T_func, T_args...>(istate, expected_offset_max, expected_offset_min, context->api_stack, context->api_value, fn, args...);
  }

  template<typename T_func, typename... T_args> void pstack_call_context_void(
    void* istate,
    int expected_offset_max,
    int expected_offset_min,
    const lua::api::compilation_context* context,
    T_func fn,
    T_args... args
  ){
    pstack_call_void<T_func, T_args...>(istate, expected_offset_max, expected_offset_min, context->api_stack, context->api_value, fn, args...);
  }

  
  template<typename T_result, typename T_func, typename... T_args> T_result pstack_call_core(
    const lua::api::core* lua_core,
    int expected_offset_max,
    int expected_offset_min,
    T_func fn,
    T_args... args
  ){
    return pstack_call<T_result, T_func, T_args...>(lua_core->istate, expected_offset_max, expected_offset_min, lua_core->context->api_stack, lua_core->context->api_value, fn, args...);
  }

  template<typename T_func, typename... T_args> void pstack_call_core_void(
    const lua::api::core* lua_core,
    int expected_offset_max,
    int expected_offset_min,
    T_func fn,
    T_args... args
  ){
    pstack_call_void<T_func, T_args...>(lua_core->istate, expected_offset_max, expected_offset_min, lua_core->context->api_stack, lua_core->context->api_value, fn, args...);
  }
}

#endif