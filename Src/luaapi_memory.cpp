#include "luaapi_memory.h"

using namespace lua::api;


#ifdef LUA_CODE_EXISTS

class _api_memory_function: public I_memory{
  public:
    int gc(void* istate, int what, int data) override{return lua_gc((lua_State*)istate, what, data);}
    lua_Alloc getallocf(void* istate, void** ud) override{return lua_getallocf((lua_State*)istate, ud);}
    void* getextraspace(void* istate) override{return lua_getextraspace((lua_State*)istate);}
    void setallocf(void* istate, lua_Alloc f, void* ud) override{return lua_setallocf((lua_State*)istate, f, ud);}
};


static _api_memory_function __api_def;

DLLEXPORT lua::api::I_memory* CPPLUA_GET_API_MEMORY_DEFINITION(){
  return &__api_def;
}

#endif // LUA_CODE_EXISTS