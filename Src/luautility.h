#ifndef LUAUTIL_HEADER
#define LUAUTIL_HEADER

#include "lua_includes.h"
#include "luaapi_compilation_context.h"
#include "luaapi_stack.h"
#include "luaapi_value.h"


namespace lua::utility{
  template<typename T_result, typename T_func, typename... T_args> T_result pstack_call(
    void* istate,
    int expected_offset_max,
    int expected_offset_min,
    lua::api::I_stack* api_s,
    lua::api::I_value* api_v,
    T_func fn,
    T_args... args
  ){
    int _prev_stack = api_s->gettop(istate);
    T_result _res = fn(args...);

    int _current_stack = api_s->gettop(istate);
    int _delta_stack = _current_stack - _prev_stack;
    if(_delta_stack > expected_offset_max)
      api_s->pop(istate, _delta_stack - expected_offset_max);
    else if(_delta_stack < expected_offset_min){
      for(int i = _delta_stack; i < expected_offset_min; i++)
        api_v->pushnil(istate);
    }

    return _res;
  }

  
  template<typename T_result, typename T_func, typename... T_args> T_result pstack_call_context(
    void* istate,
    int expected_offset_max,
    int expected_offset_min,
    const lua::api::compilation_context* context,
    T_func fn,
    T_args... args
  ){
    return pstack_call<T_result>(istate, expected_offset_max, expected_offset_min, context->api_stack, context->api_value, fn, args...);
  }
}

#endif