#ifndef LUAUTIL_HEADER
#define LUAUTIL_HEADER

#include "lua_includes.h"
#include "luaapi_stack.h"
#include "luaapi_value.h"


namespace lua::utility{
  template<lua::api::I_stack* api_s = cpplua_get_api_stack_definition(), lua::api::I_value* api_v = cpplua_get_api_value_definition(), typename T_func, typename T_result, typename... T_args> T_result pstack_call(
    void* istate,
    T_func fn,
    int expected_offset_max,
    int expected_offset_min,
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
  }
}

#endif