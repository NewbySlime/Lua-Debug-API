#ifndef LUAAPI_THREAD_HEADER
#define LUAAPI_THREAD_HEADER

#include "library_linking.h"
#include "luathread_control.h"
#include "luathread_util.h"
#include "macro_helper.h"


namespace lua::api{
  class I_thread{
    public:
      virtual lua::thread::I_mutex* require_state_mutex(void* istate) = 0;
      virtual void* require_dependent_state(void* main_istate) = 0;

      virtual void thread_dependent_enable(void* istate, bool enable) = 0;

      virtual void lock_state(void* istate) = 0;
      virtual void unlock_state(void* istate) = 0;

      virtual I_thread_handle* get_thread_handle() = 0;
  };
}


#define CPPLUA_GET_API_THREAD_DEFINITION cpplua_get_api_thread_definition
#define CPPLUA_GET_API_THREAD_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_THREAD_DEFINITION)

typedef lua::api::I_thread* (__stdcall *get_api_thread_func)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_thread* CPPLUA_GET_API_THREAD_DEFINITION();
#endif // LUA_CODE_EXISTS

#endif