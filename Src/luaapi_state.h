#ifndef LUAAPI_STATE_HEADER
#define LUAAPI_STATE_HEADER

#include "defines.h"
#include "library_linking.h"
#include "luaincludes.h"
#include "macro_helper.h"


namespace lua::api{
  class I_state{
    public:
      virtual void close(void* istate) = 0;
      virtual int load(void* istate, lua_Reader reader, void* data, const char* chunkname, const char* mode) = 0;
      virtual int loadfile(void* istate, const char* file_name, const char* mode) = 0;
      virtual void* newstate(lua_Alloc f, void* ud) = 0;
      virtual void* newstate() = 0;
  };
}


#if (__linux)
// Just to make the compiler happy
#define __stdcall 
#endif


#define CPPLUA_GET_API_STATE_DEFINITION cpplua_get_api_state_definition
#define CPPLUA_GET_API_STATE_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_STATE_DEFINITION)

typedef lua::api::I_state* (__stdcall *get_api_state_func)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_state* CPPLUA_GET_API_STATE_DEFINITION();
#endif // LUA_CODE_EXISTS

#endif