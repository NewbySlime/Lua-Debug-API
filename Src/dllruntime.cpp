#include "dllutil.h"
#include "dllruntime.h"
#include "stdint.h"

using namespace dynamic_library::util;


#if (_WIN64) || (_WIN32)

#include "Windows.h"

BOOL __cpplua_thread_util_dllevent_win(HINSTANCE, DWORD, LPVOID);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
  BOOL _result = true;
  if(!__cpplua_thread_util_dllevent_win(hinstDLL, fdwReason, lpvReserved))
    _result = false;

  return _result;
}

#else

void __cpplua_thread_util_dllevent_def(uint32_t);

#define _DEF_DLLEVENT_BATCH_CALL(event) \
  __cpplua_thread_util_dllevent_def(event);


static void _thread_create_hook();
static void _thread_delete_hook();
thread_local constructor_helper _thread_create_hooker(_thread_create_hook);
thread_local destructor_helper _thread_delete_hooker(_thread_delete_hook);


__attribute__((constructor))
static void _def_dll_starting(){
  _DEF_DLLEVENT_BATCH_CALL(def_dllevent_starting)
}

__attribute__((destructor))
static void _def_dll_finishing(){
  _DEF_DLLEVENT_BATCH_CALL(def_dllevent_finishing)
}


static void _thread_create_hook(){
  _DEF_DLLEVENT_BATCH_CALL(def_dllevent_thread_start)
}

static void _thread_delete_hook(){
  _DEF_DLLEVENT_BATCH_CALL(def_dllevent_thread_stop)
}

#endif