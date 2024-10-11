#ifndef LUALIBRARY_IOHANDLER_HEADER
#define LUALIBRARY_IOHANDLER_HEADER

#include "library_linking.h"
#include "luaI_object.h"
#include "luaobject_util.h"
#include "luavariant.h"
#include "macro_helper.h"
#include "memdynamic_management.h"
#include "stdio.h"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif


// constraint in case if the file is large
#define IO_MAXIMUM_ALL_READ_LEN 2048


namespace lua::library{
  class I_file_handler;


  // MARK: io_handler

  class I_io_handler: virtual public I_object{
    public:
      struct constructor_param{
        // Custom standard files. Can be NULL to let the IO to revert to default C/C++ Standard IO.
        // Object (memory) management shouldn't be managed by any user code, it should be handled by Lua's garbage collector.
        // But the destruction of the object must be handled by the user code by registering the destructor function to the object.

        I_file_handler* stdout_file;
        I_file_handler* stdin_file;
        I_file_handler* stderr_file;
      };

      virtual const lua::I_error_var* get_last_error() const = 0;

      virtual void close() = 0;
      virtual void flush() = 0;

      virtual bool set_input_file(I_file_handler* file) = 0;
      virtual bool set_output_file(I_file_handler* file) = 0;
      virtual bool set_error_file(I_file_handler* file) = 0;

      virtual const lua::I_table_var* get_input_file() const = 0;
      virtual const lua::I_table_var* get_output_file() const = 0;
      virtual const lua::I_table_var* get_error_file() const = 0;

      virtual size_t read(char* buffer, size_t buffer_size) = 0;
      virtual size_t read(I_string_store* pstr, size_t read_len) = 0;
      virtual size_t write(const char* buffer, size_t buffer_size) = 0;

  };

#ifdef LUA_CODE_EXISTS
  class io_handler: public I_io_handler, public lua::object::function_store, virtual public I_object{
    private:
      lua::api::core _lc;

      lua::object_var* _stdout_object;
      lua::object_var* _stdin_object;
      lua::object_var* _stderr_object;

      // Use table_var in case of the object supplied is any file handle object other than file_handler object.
      // NOTE: don't change the variables contents if the object is not a valid file handle.

      lua::table_var* _fileout_def;
      lua::table_var* _filein_def;
      lua::table_var* _fileerr_def;

      lua::table_var* _obj_data = NULL;

      lua::error_var* _last_error = NULL;

      void _clear_error();
      void _set_last_error(long long err_code, const std::string& err_msg);
      void _copy_error_from(file_handler* file);
      // ignore copying when first data is not a nil value
      void _copy_error_from(const I_vararr* data);
      bool _has_error(const I_vararr* data);
#if (_WIN64) || (_WIN32)
      void _update_last_error_winapi();
#endif

      void _fill_error_with_last(I_vararr* res);

    public:
      io_handler(const lua::api::core* lua_core, const constructor_param* param = NULL);
      ~io_handler();

      const lua::I_error_var* get_last_error() const override;

      static void close(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void flush(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void open(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      // static void popen(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void input(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void output(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void error(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void lines(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void read(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void tmpfile(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void type(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void write(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);

      // Closes output default file, will be ignored if the default output is a std output used by this IO handler.
      void close() override;
      // Flushes output and error default and/or std files.
      void flush() override;

      bool set_input_file(I_file_handler* file) override;
      bool set_output_file(I_file_handler* file) override;
      bool set_error_file(I_file_handler* file) override;

      // Returns a representation of file_handler in a Lua object.
      const lua::I_table_var* get_input_file() const override;
      // Returns a representation of file_handler in a Lua object.
      const lua::I_table_var* get_output_file() const override;
      // Returns a representation of file_handler in a Lua object.
      const lua::I_table_var* get_error_file() const override;

      size_t read(char* buffer, size_t buffer_size) override;
      size_t read(I_string_store* pstr, size_t read_len) override;
      size_t write(const char* buffer, size_t buffer_size) override;

      lua::I_debuggable_object* as_debug_object() override;
      void on_object_added(const lua::api::core* lua_core) override;

  };
#endif // LUA_CODE_EXISTS


  // MARK: file_handler

  class I_file_handler: virtual public I_object{
    public:
      enum open_op{
        open_read             = 0b0000000,
        open_write            = 0b0000001,

        // can read or write, but seperate from actual operation code
        open_special          = 0b0000010,

        open_preserve         = 0b0000100,
        open_append           = 0b0001000,

        open_binary           = 0b0010000,

        open_temporary        = 0b0100000,

        // automatically close the file when target file detects the end of file
        open_automatic_close  = 0b1000000
      };

      enum seek_opt{
        seek_begin    = 0x0,
        seek_current  = 0x1,
        seek_end      = 0x2
      };

      enum buffering_mode{
        buffering_none    = 0x0,
        buffering_full    = 0x1,
        buffering_line    = 0x2
      };


      virtual const lua::I_error_var* get_last_error() const = 0;

      virtual bool close() = 0;
      virtual bool flush() = 0;
      virtual char peek() = 0;
      virtual long seek(seek_opt opt, long offset) = 0;
      virtual size_t read(char* buffer, size_t buffer_size) = 0;
      virtual size_t write(const char* buffer, size_t buffer_size) = 0;
      virtual void set_buffering_mode(buffering_mode mode) = 0;

      virtual bool already_closed() const = 0;
      virtual bool end_of_file_reached() const = 0;
      virtual size_t get_remaining_read() const = 0;

      virtual bool can_write() const = 0;
      virtual bool can_read() const = 0;
       
  };

#ifdef LUA_CODE_EXISTS
  class file_handler: public I_file_handler, public lua::object::function_store, virtual public I_object, virtual public I_debuggable_object{
    private:
      int _op_code;

#if (_WIN64) || (_WIN32)
      HANDLE _hfile = NULL; // also used as pipe write

      bool _is_pipe_handle = true;
      bool _own_handle = true;
#endif

      long _minimal_write_offset = 0;

      buffering_mode _buffering_mode = buffering_none;

      lua::api::core _lc;

      lua::error_var* _last_error = NULL;

      // NOTE: Reason for using metadata (reference) table is to pass it as the upvalues of the callbacks of certain iterating functions.
      //  By using reference, whenever the actual object is deleted, the referenced table's data can be cleared. Hence preventing it to run using NULL object.
      //  - - -
      //  The reference can also be used by any functions when it has to return itself (object table).
      //  This approach will let the Lua's garbage collector to not approve this object to the next cycle. Unless, the Lua program (lua_State) is destroyed, or when the file has been closed.
      // WARN: Do not use object_var for referencing as some functions will have data (upvalue) about a pointer of the object, which at the context, we don't know if the object has been deleted or not.

      lua::table_var* _object_metadata = NULL; // created on object initialization
      lua::table_var* _object_table = NULL; // created on object pushed to Lua (see on_object_added)


      void _clear_error();
      void _set_last_error(long long err_code, const std::string& err_msg);
#if (_WIN64) || (_WIN32)
      void _update_last_error_winapi();
#endif

      void _initiate_reference();
      void _delete_object_metadata();
      // prefered calling sequence: _delete_object_table -> _delete_object_metadata
      void _delete_object_table();

      // filter to skip
      void _skip_str(bool(*filter_cb)(char ch));
      // filter to stop read
      // NOTE: the function will insert null-termination character, (buffer) should at least larger than (buflen) by 1.
      // Returns -1 if the file has reached end of file
      int _read_str(char* buffer, size_t buflen, bool(*filter_cb)(char ch) = NULL);

      // This function is used for appending mode (constrained by _minimal_write_offset).
      void _check_write_pos();

      void _fill_error_with_last(I_vararr* result);

      static std::string _get_pointer_str(const void* pointer);

      static int _lua_line_cb(const lua::api::core* lua_core);

    public:
      file_handler(const lua::api::core* lua_core, const std::string& path, int op = open_read);
#if (_WIN64) || (_WIN32)
      file_handler(const lua::api::core* lua_core, HANDLE pipe_handle, bool is_output = true);
#endif
      ~file_handler();

      const lua::I_error_var* get_last_error() const override;

      static bool check_object_validity(const I_table_var* tvar);

      static void close(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void flush(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void lines(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void read(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void seek(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void setvbuf(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);
      static void write(lua::I_object* obj, const lua::I_vararr* args, lua::I_vararr* res);

      bool close() override;
      bool flush() override;
      char peek() override;
      long seek(seek_opt opt, long offset) override;
      size_t read(char* buffer, size_t buffer_size) override;
      size_t write(const char* buffer, size_t buffer_size) override;
      void set_buffering_mode(buffering_mode mode) override;

      bool already_closed() const override;
      bool end_of_file_reached() const override;
      size_t get_remaining_read() const override;

      bool can_write() const override;
      bool can_read() const override;

      lua::I_debuggable_object* as_debug_object() override;
      void on_object_added(const lua::api::core* lua_core) override;

  };

  
  struct io_handler_api_constructor_data{
    const lua::api::core* lua_core;
    const io_handler::constructor_param* param = NULL;
  };

  struct file_handler_api_constructor_data{
    const lua::api::core* lua_core;

    bool use_pipe = false;

    const char* path;
    int open_mode = file_handler::open_read;

#if (_WIN32) || (_WIN64)
    HANDLE pipe_handle;
    bool is_output = true;
#endif
  };
#endif // LUA_CODE_EXISTS

}


#define CPPLUA_LIBRARY_SET_IO_HANDLER_MEMORY_MANAGEMENT_CONFIG cpplua_library_set_io_handler_memory_management_config
#define CPPLUA_LIBRARY_SET_IO_HANDLER_MEMORY_MANAGEMENT_CONFIG_STR MACRO_TO_STR_EXP(CPPLUA_LIBRARY_SET_IO_HANDLER_MEMORY_MANAGEMENT_CONFIG)

#define CPPLUA_LIBRARY_CREATE_IO_HANDLER cpplua_library_create_io_handler
#define CPPLUA_LIBRARY_CREATE_IO_HANDLER_STR MACRO_TO_STR_EXP(CPPLUA_LIBRARY_CREATE_IO_HANDLER)

#define CPPLUA_LIBRARY_DELETE_IO_HANDLER cpplua_library_delete_io_handler
#define CPPLUA_LIBRARY_DELETE_IO_HANDLER_STR MACRO_TO_STR_EXP(CPPLUA_LIBRARY_DELETE_IO_HANDLER)

#define CPPLUA_LIBRARY_CREATE_FILE_HANDLER cpplua_library_create_file_handler
#define CPPLUA_LIBRARY_CREATE_FILE_HANDLER_STR MACRO_TO_STR_EXP(CPPLUA_LIBRARY_CREATE_FILE_HANDLER)

#define CPPLUA_LIBRARY_DELETE_FILE_HANDLER cpplua_library_delete_file_handler
#define CPPLUA_LIBRARY_DELETE_FILE_HANDLER_STR MACRO_TO_STR_EXP(CPPLUA_LIBRARY_DELETE_FILE_HANDLER)

typedef lua::library::I_io_handler* (__stdcall *create_library_io_handler_func)(const lua::library::io_handler_api_constructor_data* data);
typedef void (__stdcall *delete_library_io_handler_func)(lua::library::I_io_handler* handler);

typedef lua::library::I_file_handler* (__stdcall *create_library_file_handler_func)(const lua::library::file_handler_api_constructor_data* data);
typedef void (__stdcall *delete_library_file_handler_func)(lua::library::I_file_handler* handler);

typedef void (__stdcall *set_library_io_handler_memory_management_config)(const ::memory::memory_management_config* config);

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::library::I_io_handler* CPPLUA_LIBRARY_CREATE_IO_HANDLER(const lua::library::io_handler_api_constructor_data* data);
DLLEXPORT void CPPLUA_LIBRARY_DELETE_IO_HANDLER(lua::library::I_io_handler* handler);

DLLEXPORT lua::library::I_file_handler* CPPLUA_LIBRARY_CREATE_FILE_HANDLER(const lua::library::file_handler_api_constructor_data* data);
DLLEXPORT void CPPLUA_LIBRARY_DELETE_FILE_HANDLER(lua::library::I_file_handler* handler);

DLLEXPORT void CPPLUA_LIBRARY_SET_IO_HANDLER_MEMORY_MANAGEMENT_CONFIG(const ::memory::memory_management_config* config);
#endif // LUA_CODE_EXISTS

#endif