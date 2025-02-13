#ifndef LUAAPI_DEBUG_HEADER
#define LUAAPI_DEBUG_HEADER

#include "defines.h"
#include "luadebug_execution_flow.h"
#include "luadebug_file_info.h"
#include "luadebug_hook_handler.h"
#include "luadebug_variable_watcher.h"
#include "library_linking.h"
#include "luaincludes.h"
#include "macro_helper.h"


namespace lua::api{
  class I_debug{
    public:
      virtual lua_Hook gethook(void* istate) = 0;
      virtual int gethookcount(void* istate) = 0;
      virtual int gethookmask(void* istate) = 0;
      virtual int getinfo(void* istate, const char* what, void* dbg_obj) = 0;
      virtual const char* getlocal(void* istate, const void* dbg_obj, int n) = 0;
      virtual int getstack(void* istate, int level, void* dbg_obj) = 0;
      virtual const char* getupvalue(void* istate, int funcindex, int n) = 0;
      virtual void sethook(void* istate, lua_Hook f, int mask, int count) = 0;
      virtual const char* setlocal(void* istate, const void* dbg_obj, int n) = 0;
      virtual const char* setupvalue(void* istate, int funcindex, int n) = 0;
      virtual void* upvalueid(void* istate, int funcindex, int n) = 0;
      virtual void upvaluejoin(void* istate, int fidx1, int n1, int fidx2, int n2) = 0;

      virtual void* create_lua_debug_obj() = 0;
      virtual void delete_lua_debug_obj(void* dbg_obj) = 0;

      virtual lua::debug::I_variable_watcher* create_variable_watcher(void* istate) = 0;
      virtual void delete_variable_watcher(lua::debug::I_variable_watcher* object) = 0;

      virtual lua::debug::I_file_info* create_file_info(const char* file_path) = 0;
      virtual void delete_file_info(lua::debug::I_file_info* obj) = 0;
  };
}


#define CPPLUA_GET_API_DEBUG_DEFINITION cpplua_get_api_debug_definition
#define CPPLUA_GET_API_DEBUG_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_DEBUG_DEFINITION)

typedef lua::api::I_debug* (__stdcall *get_api_debug_func)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_debug* CPPLUA_GET_API_DEBUG_DEFINITION();
#endif // LUA_CODE_EXISTS

#endif