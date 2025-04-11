#ifndef LUA_DEBUG_INFO_HEADER
#define LUA_DEBUG_INFO_HEADER

#include "library_linking.h"
#include "luaapi_core.h"
#include "macro_helper.h"
#include "string_store.h"


namespace lua::debug{
  class I_function_debug_info{
    public:
      virtual bool get_function_name(I_string_store* str) const = 0;
      virtual bool get_file_path(I_string_store* str) const = 0;
      virtual int get_line_defined() const = 0;
      virtual int get_last_line_defined() const = 0;

      virtual bool is_lua_function() const = 0;
  };

  class function_debug_info: public I_function_debug_info{
    private:
      std::string _function_name = "";
      std::string _file_path = "";
      int _line_defined = -1;
      int _last_line_defined = -1;

      bool _is_lua_function = false;

    public:
      function_debug_info(){}
      function_debug_info(const I_function_debug_info* obj);
#ifdef LUA_CODE_EXISTS
      function_debug_info(lua_State* state, int idx);
#endif

      bool get_function_name(I_string_store* str) const override;
      bool get_file_path(I_string_store* str) const override;
      int get_line_defined() const override;
      int get_last_line_defined() const override;

      bool is_lua_function() const override;
  };
}


#define CPPLUA_CREATE_FUNCTION_DEBUG_INFO cpplua_create_function_debug_info
#define CPPLUA_CREATE_FUNCTION_DEBUG_INFO_STR MACRO_TO_STR_EXP(CPPLUA_CREATE_FUNCTION_DEBUG_INFO)

#define CPPLUA_DELETE_FUNCTION_DEBUG_INFO cpplua_delete_function_debug_info
#define CPPLUA_DELETE_FUNCTION_DEBUG_INFO_STR MACRO_TO_STR_EXP(CPPLUA_DELETE_FUNCTION_DEBUG_INFO)

typedef lua::debug::I_function_debug_info* (__stdcall *fdi_create_func)(lua_State* state, int idx);
typedef void (__stdcall *fdi_delete_func)(lua::debug::I_function_debug_info* obj);

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::debug::I_function_debug_info* CPPLUA_CREATE_FUNCTION_DEBUG_INFO(lua_State* state, int idx);
DLLEXPORT void CPPLUA_DELETE_FUNCTION_DEBUG_INFO(lua::debug::I_function_debug_info* obj);
#endif // LUA_CODE_EXISTS

#endif