// see lbaselib.c for override global function
#ifndef LUASTREAM_PRINT_OVERRIDE_HEADER
#define LUASTREAM_PRINT_OVERRIDE_HEADER

#include "I_debug_user.h"
#include "lua_includes.h"

#if _WIN64
#include "Windows.h"
#endif


namespace lua::global{
  class print_override: I_debug_user{
    private:
#if _WIN64
      HANDLE _event_handle;
      HANDLE _pipe_write;
      HANDLE _pipe_read;
#endif

    I_logger* _logger;

    lua_State* _this_state;

    void _bind_global_function();

    static void _set_bind_obj(print_override* obj, lua_State* state);
    static int _on_print_static(lua_State* state);

    public:
      print_override(lua_State* state);
      ~print_override();

      static print_override* get_attached_object(lua_State* state);

      unsigned long read_n(char* buffer, unsigned long buffer_size);
      std::string read_all();

      unsigned long peek_n(char* buffer, unsigned long buffer_size);
      std::string peek_all();

      unsigned long available_bytes();

#if _WIN64
      HANDLE get_event_handle();
#endif

      void set_logger(I_logger* logger) override;
  };
}

#endif