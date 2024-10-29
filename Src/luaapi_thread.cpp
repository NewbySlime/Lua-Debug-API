#include "luaapi_thread.h"


#ifdef LUA_CODE_EXISTS

using namespace lua;
using namespace lua::api;
using namespace lua::thread;


class _api_thread_function: public I_thread{
  public:
    I_mutex* require_state_mutex(void* istate) override{return lua::thread::require_state_mutex((lua_State*)istate);}
    void* require_dependent_state(void* main_istate) override{return lua::thread::require_dependent_state((lua_State*)main_istate);}

    void thread_dependent_enable(void* istate, bool enable) override{lua::thread::thread_dependent_enable((lua_State*)istate, enable);}

    void lock_state(void* istate) override{lua::thread::lock_state((lua_State*)istate);}
    void unlock_state(void* istate) override{lua::thread::unlock_state((lua_State*)istate);}

    I_thread_handle* get_thread_handle() override{return lua::get_thread_handle();}
};


static _api_thread_function __api_def;

DLLEXPORT I_thread* CPPLUA_GET_API_THREAD_DEFINITION(){
  return &__api_def;
}

#endif // LUA_CODE_EXISTS