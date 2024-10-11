#include "luainternal_storage.h"
#include "luamemory_util.h"
#include "luastate_util.h"

using namespace lua;
using namespace lua::internal;
using namespace lua::memory;


#if LUA_CODE_EXISTS

lua_State* lua::newstate(lua_Alloc f, void* ud){
  lua_State* _result = lua_newstate(f, ud);
  initiate_internal_storage(_result);
  return _result;
}

lua_State* lua::newstate(){
  lua_State* _result = newstate(memory_manager_as_lua_alloc(), NULL);
  lua_setallocf(_result, memory_manager_as_lua_alloc(), NULL);
  return _result;
}

#endif