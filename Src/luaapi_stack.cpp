#include "luaapi_stack.h"

using namespace lua::api;


static class _api_stack_function: public I_stack{
  public:
    int absindex(void* istate, int idx) override{return lua_absindex((lua_State*)istate, idx);}
    int checkstack(void* istate, int n) override{return lua_checkstack((lua_State*)istate, n);}
    void copy(void* istate, int fromidx, int toidx) override{lua_copy((lua_State*)istate, fromidx, toidx);}
    int gettop(void* istate) override{return lua_gettop((lua_State*)istate);}
    void insert(void* istate, int idx) override{return lua_insert((lua_State*)istate, idx);}
    void pop(void* istate, int n) override{lua_pop((lua_State*)istate, n);}
    void pushvalue(void* istate, int objidx) override{lua_pushvalue((lua_State*)istate, objidx);}
    void remove(void* istate, int idx) override{lua_remove((lua_State*)istate, idx);}
    void replace(void* istate, int idx) override{lua_replace((lua_State*)istate, idx);}
    void rotate(void* istate, int idx, int n) override{lua_rotate((lua_State*)istate, idx, n);}
    void settop(void* istate, int idx) override{lua_settop((lua_State*)istate, idx);} 
};


static _api_stack_function __api_def;

DLLEXPORT lua::api::I_stack* CPPLUA_GET_API_STACK_DEFINITION(){
  return &__api_def;
}