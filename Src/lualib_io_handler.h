#ifndef LUALIB_IO_HANDLER_HEADER
#define LUALIB_IO_HANDLER_HEADER

#include "luaI_object.h"
#include "luaobject_helper.h"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif


namespace lua::lib{
  class file_Handler;

  class io_handler: public lua::object::function_store, virtual public I_object{
    public:
      struct constructor_param{
        // Custom standard files. Can be NULL to let the IO to revert to default C/C++ Standard IO.
        // Objects management shouldn't be managed by any user code, it should be handled by Lua's garbage collector.

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


  class file_handler: public lua::object::function_store, virtual public I_object{
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
        seek_current  = 0x2
      };


    private:
      int _op_code;

#if (_WIN64) || (_WIN32)
      HANDLE _hfile = NULL; // also used as pipe write

      bool _own_handle = true;
#endif

      bool _is_appending = 0;

      error_var* _last_error = NULL;


      void _close_file();
      void _flush_file();
      bool _already_closed();

      void _clear_error();
      void _set_last_error(long long err_code, const std::string& err_msg);

      // filter to skip
      void _skip_str(bool(*filter_cb)(char ch));
      // filter to stop read
      std::string _read_str(bool(*filter_cb)(char ch));
      std::string _read_bytes(size_t count);

      void _seek(seek_opt opt, int offset);

      void _fill_error_with_last(I_vararr* result);

    public:
      file_handler(const std::string& path, int op = open_read);
#if (_WIN64) || (_WIN32)
      file_handler(HANDLE pipe_handle, bool is_output = true);
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

      bool end_of_file_reached();
      size_t get_remaining_read();

      I_debuggable_object* as_debug_object() override;
      void on_object_added(void* istate) override;
  };
}

#endif