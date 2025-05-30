#ifndef LUAAPI_VARIANT_UTIL_HEADER
#define LUAAPI_VARIANT_UTIL_HEADER

#include "defines.h"
#include "luavariant.h"
#include "string_store.h"


namespace lua::api{
  class I_variant_util{
    public:
      virtual I_variant* to_variant(void* istate, int stack_idx) = 0;
      virtual I_variant* to_variant_fglobal(void* istate, const char* global_name) = 0;
      virtual I_variant* to_variant_ref(void* istate, int stack_idx) = 0;

      virtual void to_string(void* istate, int stack_idx, I_string_store* result) = 0;

      virtual void set_special_type(void* istate, int stack_idx, int new_type) = 0;
      virtual int get_special_type(void* istate, int stack_idx) = 0;

      virtual void set_global(void* istate, const char* global_name, I_variant* var) = 0;

      // Might return error_var if an error happens.
      virtual I_variant* load_file_as_function(const char* file_path) = 0;

      virtual I_variant* create_variant_copy(const I_variant* var) = 0;
      virtual void delete_variant(const I_variant* var) = 0;

      virtual void set_default_logger(I_logger* logger) = 0;
  };
}


// MARK: DLL functions

#define CPPLUA_GET_API_VARIANT_UTIL_DEFINITION cpplua_get_api_variant_util_definition
#define CPPLUA_GET_API_VARIANT_UTIL_DEFINITION_STR MACRO_TO_STR_EXP(CPPLUA_GET_API_VARIANT_UTIL_DEFINITION)

typedef lua::api::I_variant_util* (__stdcall *get_api_variant_util_func)();

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::api::I_variant_util* CPPLUA_GET_API_VARIANT_UTIL_DEFINITION();
#endif // LUA_CODE_EXISTS

#endif