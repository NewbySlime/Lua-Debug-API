#ifndef LUAAPI_RUNTIME_HEADER
#define LUAAPI_RUNTIME_HEADER

#include "defines.h"
#include "luaglobal_print_override.h"
#include "lualibrary_loader.h"
#include "luaruntime_handler.h"
#include "library_linking.h"
#include "macro_helper.h"


namespace lua::api{
  class I_runtime{
    public:
      // TODO: add create thread_control
    
      virtual lua::global::I_print_override* create_print_override(void* istate) = 0;
      virtual void delete_print_override(lua::global::I_print_override* object) = 0;
      virtual lua::I_library_loader* create_library_loader(void* istate) = 0;
      virtual void delete_library_loader(lua::I_library_loader* object) = 0;
      virtual lua::I_runtime_handler* create_runtime_handler(const char* lua_path, bool load_library) = 0;
      virtual void delete_runtime_handler(lua::I_runtime_handler* object) = 0;

      virtual lua::global::I_print_override* get_print_override_instance(void* istate) = 0;
      virtual lua::I_library_loader* get_library_loader_instance(void* istate) = 0;
      virtual lua::I_runtime_handler* get_runtime_handler_instance(void* istate) = 0;
  };
}


// MARK: DLL definitions

#define CPPLUA_GET_API_RUNTIME_DEFINITION cpplua_get_api_runtime_definition
#define CPPLUA_GET_API_RUNTIME_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_RUNTIME_DEFINITION)

typedef lua::api::I_runtime* (__stdcall *get_api_runtime_definition)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_runtime* CPPLUA_GET_API_RUNTIME_DEFINITION();
#endif

#endif