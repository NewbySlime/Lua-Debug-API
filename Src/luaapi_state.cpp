#include "luaapi_state.h"
#include "luastate_util.h"

using namespace lua::api;

#ifdef LUA_CODE_EXISTS

class _api_state_function: public I_state{
  public:
    void close(void* istate) override{return lua_close((lua_State*)istate);}
    int load(void* istate, lua_Reader reader, void* data, const char* chunkname, const char* mode) override{return lua_load((lua_State*)istate, reader, data, chunkname, mode);}
    int loadfile(void* istate, const char* file_name, const char* mode) override{return luaL_loadfilex((lua_State*)istate, file_name, mode);}
    void* newstate(lua_Alloc f, void* ud) override{return lua_newstate(f, ud);}
    void* newstate() override{return lua::newstate();}
};


static _api_state_function __api_def;

DLLEXPORT lua::api::I_state* CPPLUA_GET_API_STATE_DEFINITION(){
  return &__api_def;
}

#endif // LUA_CODE_EXISTS