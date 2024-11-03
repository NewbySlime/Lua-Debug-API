#if (_WIN64) || (_WIN32)

#include "Windows.h"


BOOL __cpplua_thread_util_dllevent(HINSTANCE, DWORD, LPVOID);
BOOL __cpplua_thread_control_dllevent(HINSTANCE, DWORD, LPVOID);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
  BOOL _result = true;
  if(!__cpplua_thread_util_dllevent(hinstDLL, fdwReason, lpvReserved))
    _result = false;

  if(!__cpplua_thread_control_dllevent(hinstDLL, fdwReason, lpvReserved))
    _result = false;

  return _result;
}

#endif