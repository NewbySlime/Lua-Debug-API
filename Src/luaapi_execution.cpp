#include "luaapi_execution.h"

using namespace lua::api;


#ifdef LUA_CODE_EXISTS

class _api_execution_function: public I_execution{
  public:
    void call(void* istate, int nargs, int nresults) override{lua_call((lua_State*)istate, nargs, nresults);}
    void callk(void* istate, int nargs, int nresults, lua_KContext ctx, lua_KFunction k) override{lua_callk((lua_State*)istate, nargs, nresults, ctx, k);}
    int pcall(void* istate, int nargs, int nresults, int msgh) override{return lua_pcall((lua_State*)istate, nargs, nresults, msgh);}
    int pcallk(void* istate, int nargs, int nresults, int msgh, lua_KContext ctx, lua_KFunction k) override{return lua_pcallk((lua_State*)istate, nargs, nresults, msgh, ctx, k);}
    int resume(void* istate, void* ifrom, int nargs) override{return lua_resume((lua_State*)istate, (lua_State*)ifrom, nargs);}
    int status(void* istate) override{return lua_status((lua_State*)istate);}
    int yield(void* istate, int nresults) override{return lua_yield((lua_State*)istate,nresults);}
    int yieldk(void* istate, int nresults, lua_KContext ctx, lua_KFunction k) override{return lua_yieldk((lua_State*)istate, nresults, ctx, k);}
};


static _api_execution_function __api_def;

DLLEXPORT lua::api::I_execution* CPPLUA_GET_API_EXECUTION_DEFINITION(){
  return &__api_def;
}

#endif // LUA_CODE_EXISTS