#include "luaapi_state.h"

using namespace lua::api;


class _api_state_function: public I_state{
  public:
    void close(void* istate) override{return lua_close((lua_State*)istate);}
    int load(void* istate, lua_Reader reader, void* data, const char* chunkname, const char* mode) override{return lua_load((lua_State*)istate, reader, data, chunkname, mode);}
    void* newstate(lua_Alloc f, void* ud) override{return lua_newstate(f, ud);}
};


static _api_state_function __api_def;

DLLEXPORT lua::api::I_state* CPPLUA_GET_API_STATE_DEFINITION(){
  return &__api_def;
}