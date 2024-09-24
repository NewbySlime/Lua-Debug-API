#ifndef LUALIB_IO_HANDLER_HEADER
#define LUALIB_IO_HANDLER_HEADER

#include "luaI_object.h"
#include "luaobject_helper.h"
#include "stdio.h"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif


// constraint in case if the file is large
#define IO_MAXIMUM_ALL_READ_LEN 2048


namespace lua::lib{
  class file_Handler;

  class io_handler: public lua::object::function_store, virtual public I_object{
    public:
      struct constructor_param{
        // Custom standard files. Can be NULL to let the IO to revert to default C/C++ Standard IO.
        // Object (memory) management shouldn't be managed by any user code, it should be handled by Lua's garbage collector.

        file_handler* stdout_file;
        file_handler* stdin_file;
        file_handler* stderr_file;
      };

    public:
      io_handler(const constructor_param* param);
      io_handler();
      ~io_handler();

      static void close(I_object* obj, const I_vararr* args, I_vararr* res);
      static void flush(I_object* obj, const I_vararr* args, I_vararr* res);
      static void input(I_object* obj, const I_vararr* args, I_vararr* res);
      static void lines(I_object* obj, const I_vararr* args, I_vararr* res);
      static void open(I_object* obj, const I_vararr* args, I_vararr* res);
      static void output(I_object* obj, const I_vararr* args, I_vararr* res);
      static void popen(I_object* obj, const I_vararr* args, I_vararr* res);
      static void read(I_object* obj, const I_vararr* args, I_vararr* res);
      static void tmpfile(I_object* obj, const I_vararr* args, I_vararr* res);
      static void type(I_object* obj, const I_vararr* args, I_vararr* res);
      static void write(I_object* obj, const I_vararr* args, I_vararr* res);
  };


  class file_handler: public lua::object::function_store, virtual public I_object, virtual public I_debuggable_object{
    public:
      enum open_op{
        open_read             = 0b000000,
        open_write            = 0b000001,

        // can read or write, but seperate from actual operation code
        open_special          = 0b000010,

        open_preserve         = 0b000100,
        open_append           = 0b001000,

        open_binary           = 0b010000,

        open_temporary        = 0b100000
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


    private:
      int _op_code;

#if (_WIN64) || (_WIN32)
      HANDLE _hfile = NULL; // also used as pipe write

      bool _is_pipe_handle = true;
      bool _own_handle = true;
#endif

      long _minimal_write_offset = 0;

      buffering_mode _buffering_mode = buffering_none;

      error_var* _last_error = NULL;

      table_var* _object_metadata = NULL; // created on object initialization
      table_var* _object_table = NULL; // created on object pushed to Lua (see on_object_added)

      void* _lua_state;
      const compilation_context* _current_context;

      void _clear_error();
      void _set_last_error(long long err_code, const std::string& err_msg);
#if (_WIN64) || (_WIN32)
      void _update_last_error_winapi();
#endif

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

      // NOTE: Reason for using metadata (reference) table is to pass it as the upvalues of the callbacks of certain iterating functions.
      //  By using reference, whenever the actual object is deleted, the referenced table's data can be cleared. Hence preventing it to run using NULL object.
      //  - - -
      //  The reference can also be used by any functions when it has to return itself (object table).
      //  This approach will create a temporary memory leak where the Lua's garbage collector will not approve this object to the next cycle. Unless, the Lua program (lua_State) is destroyed, or when the file has been closed.

      // get or create global table that holds references metadata to file_handler objects
      // Lua: returns (pushes) a referencing global table
      static void _require_global_table(void* istate, const lua::api::compilation_context* context);
      // create metadata (will be skipped if metadata already existed)
      // C++: returns table reference to the metadata
      // Lua: returns (pushes) nothing
      static table_var* _create_metadata(void* istate, const lua::api::compilation_context* context, file_handler* obj);
      // delete metadata (will also check if metadata existed or not)
      // Lua: returns (pushes) nothing
      static void _delete_metadata(void* istate, const lua::api::compilation_context* context, file_handler* obj);

      static std::string _get_pointer_str(const void* pointer);

      // LUA PARAM: metadata table as the first parameter
      static file_handler* _get_file_handler(void* istate, const lua::api::compilation_context* context);

      static int _lua_line_cb(void* istate, const lua::api::compilation_context* context);

    public:
      file_handler(void* istate, const lua::api::compilation_context* context, const std::string& path, int op = open_read);
#if (_WIN64) || (_WIN32)
      file_handler(void* istate, const lua::api::compilation_context* context, HANDLE pipe_handle, bool is_output = true);
#endif
      ~file_handler();

      const I_error_var* get_last_error() const;

      static void close(I_object* obj, const I_vararr* args, I_vararr* res);
      static void flush(I_object* obj, const I_vararr* args, I_vararr* res);
      static void lines(I_object* obj, const I_vararr* args, I_vararr* res);
      static void read(I_object* obj, const I_vararr* args, I_vararr* res);
      static void seek(I_object* obj, const I_vararr* args, I_vararr* res);
      static void setvbuf(I_object* obj, const I_vararr* args, I_vararr* res);
      static void write(I_object* obj, const I_vararr* args, I_vararr* res);

      bool close();
      bool flush();
      char peek();
      long seek(seek_opt opt, long offset);
      size_t read(char* buffer, size_t buffer_size);
      size_t write(const char* buffer, size_t buffer_size);
      void set_buffering_mode(buffering_mode mode);

      bool already_closed() const;
      bool end_of_file_reached() const;
      size_t get_remaining_read() const;

      bool can_write() const;
      bool can_read() const;

      I_debuggable_object* as_debug_object() override;
      void on_object_added(void* istate, const lua::api::compilation_context* context) override;
  };
}

#endif