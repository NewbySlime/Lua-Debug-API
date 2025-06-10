#ifndef LUADEBUG_FILE_INFO_HEADER
#define LUADEBUG_FILE_INFO_HEADER

#include "defines.h"
#include "luaapi_core.h"
#include "luaincludes.h"
#include "luavariant.h"
#include "macro_helper.h"

#include "string"
#include "vector"


namespace lua::debug{
  class I_file_info{
    public:
      virtual void set_file_path(const char* file_path) = 0;
      virtual const char* get_file_path() = 0;

      virtual void refresh_info() = 0;
    
      virtual long get_line_count() = 0;

      virtual bool is_line_valid(long line_idx) = 0;
      virtual long get_line_idx(long line_idx) = 0;

      virtual const lua::I_error_var* get_last_error() = 0;
  };

#ifdef LUA_CODE_EXISTS
  class file_info: public I_file_info{
    private:
      struct line_info{
        long buffer_idx_start;
        bool valid_line;
      };

      error_var* _err_data = NULL;

      std::string _file_path;
      std::vector<line_info> _lines_info;

      lua_State* _lstate;

      void _set_error_data(long long errcode, const char* error_str);
      void _copy_error_data();
      void _delete_error_data();

      void _update_file_info(char* buffer);
      // returns true if found something
      // NOTE: buffer should be 1+ more bigger than the actual string.
      bool _check_code_buffer(char* buffer, size_t buffer_size, long start_line, long finish_line, long recur_idx = 0);

    public:
      file_info(const char* file_path);
      ~file_info();

      void set_file_path(const char* file_path) override;
      const char* get_file_path() override;

      void refresh_info() override;

      long get_line_count() override;

      bool is_line_valid(long line_idx) override;
      long get_line_idx(long line_idx) override;

      const lua::I_error_var* get_last_error() override;
  };
#endif
}


#if (__linux)
// Just to make the compiler happy
#define __stdcall 
#endif


#define CPPLUA_CREATE_FILE_INFO cpplua_create_file_info
#define CPPLUA_CREATE_FILE_INFO_STR MACRO_TO_STR_EXP(CPPLUA_CREATE_FILE_INFO)

#define CPPLUA_DELETE_FILE_INFO cpplua_delete_file_info
#define CPPLUA_DELETE_FILE_INFO_STR MACRO_TO_STR_EXP(CPPLUA_DELETE_FILE_INFO)

typedef lua::debug::I_file_info* (__stdcall *fi_create_func)(const char* file_path);
typedef void (__stdcall *fi_delete_func)(lua::debug::I_file_info* obj);

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::debug::I_file_info* CPPLUA_CREATE_FILE_INFO(const char* file_path);
DLLEXPORT void CPPLUA_DELETE_FILE_INFO(lua::debug::I_file_info* obj);
#endif // LUA_CODE_EXISTS

#endif