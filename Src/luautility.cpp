#include "luaapi_compilation_context.h"
#include "luaapi_util.h"
#include "luautility.h"

using namespace lua;
using namespace lua::api;
using namespace lua::utility;


bool lua::utility::verify_lua_version(const lua::api::core* lc1, const lua::api::core* lc2){
  return verify_lua_version(lc1->context, lc2->context);
}

bool lua::utility::verify_lua_version(const compilation_context* c1, const compilation_context* c2){
  return abs(*c1->api_util->version(NULL) - *c2->api_util->version(NULL)) < 0.001;
}


bool lua::utility::check_same_runtime(const core* lc1, const core* lc2){
  return get_main_thread(lc1) == get_main_thread(lc2);
}

void* lua::utility::get_main_thread(const core* lc){
  // Get registry table s+1
  lc->context->api_stack->pushvalue(lc->istate, LUA_REGISTRYINDEX);
  
  lc->context->api_value->pushinteger(lc->istate, LUA_RIDX_MAINTHREAD);
  lc->context->api_value->gettable(lc->istate, -2);
  void* _result = lc->context->api_value->tothread(lc->istate, -1);

  // Pop registry table and main thread value
  lc->context->api_stack->pop(lc->istate, 2);

  return _result;
}


#ifdef LUA_CODE_EXISTS

bool lua::utility::verify_lua_version(const core* lc2){
  return verify_lua_version(lc2->context);
}

bool lua::utility::verify_lua_version(const compilation_context* c2){
  return verify_lua_version(cpplua_get_api_compilation_context(), c2);
}


lua_State* lua::utility::get_main_thread(lua_State* state){
  core _lc = as_lua_api_core(state);
  return (lua_State*)get_main_thread(&_lc);
}


lua::api::core lua::utility::as_lua_api_core(lua_State* state){
  core _result;
    _result.istate = state;
    _result.context = cpplua_get_api_compilation_context();

  return _result;
}

#endif