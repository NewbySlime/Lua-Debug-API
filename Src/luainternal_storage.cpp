#include "luainternal_storage.h"
#include "luaapi_value.h"
#include "luaapi_stack.h"

#define LUA_INTERNAL_STORAGE_NAME "__clua_internal_storage"


#ifdef LUA_CODE_EXISTS

void lua::internal::initiate_internal_storage(lua_State* state){
  if(lua_getglobal(state, LUA_INTERNAL_STORAGE_NAME) == LUA_TNIL){
    lua_newtable(state);
    lua_setglobal(state, LUA_INTERNAL_STORAGE_NAME);
  }

  lua_pop(state, 1);
}


void lua::internal::require_internal_storage(lua_State* state){
  initiate_internal_storage(state);
  lua_getglobal(state, LUA_INTERNAL_STORAGE_NAME);
}

#endif // LUA_CODE_EXISTS