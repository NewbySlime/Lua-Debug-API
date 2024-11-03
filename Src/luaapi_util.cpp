#include "luaapi_util.h"
#include "luautility.h"

using namespace lua::api;


#ifdef LUA_CODE_EXISTS

class _api_util_function: public I_util{
  public:
    int absindex(void* istate, int idx) override{return lua_absindex((lua_State*)istate, idx);}
    lua_CFunction atpanic(void* istate, lua_CFunction panicf) override{return lua_atpanic((lua_State*)istate, panicf);}
    int dump(void* istate, lua_Writer writer, void* data, int strip) override{return lua_dump((lua_State*)istate, writer, data, strip);}
    int error(void* istate) override{return lua_error((lua_State*)istate);}
    const lua_Number* version(void* istate) override{return lua_version((lua_State*)istate);}

    void* get_main_thread(void* istate) override{return lua::utility::get_main_thread((lua_State*)istate);}
};


static _api_util_function __api_def;

DLLEXPORT lua::api::I_util* CPPLUA_GET_API_UTIL_DEFINITION(){
  return &__api_def;
}

#endif // LUA_CODE_EXISTS