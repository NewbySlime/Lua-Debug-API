#ifndef LUARUNTIME_HANDLER_HEADER
#define LUARUNTIME_HANDLER_HEADER

#include "I_debug_user.h"
#include "I_logger.h"
#include "lua_includes.h"
#include "luadebug_hookhandler.h"
#include "luadebug_executionflow.h"
#include "luafunction_database.h"

#if _WIN64
#include "Windows.h"
#endif


namespace lua{
  // NOTE: lua_State shouldn't be destroyed in this class' lifetime
  class runtime_handler: public I_debug_user{
    public:
      typedef void (*execution_context)(void* data);

    private:
      I_logger* _logger;

      lua_State* _state;

      bool _stop_thread = false;

      bool _create_own_lua_state = false;

#if _WIN64
      HANDLE _thread_handle = NULL;

      struct _t_entry_point_data{
        execution_context cb;
        void* cbdata;
      };

      static DWORD _thread_entry_point(LPVOID data);
#endif

      lua::debug::hook_handler* _hook_handler = NULL;
      lua::debug::execution_flow* _execution_flow = NULL;

      lua::func_db* _function_database = NULL;

      void _hookcb();

      void _initiate_class();

      static void _set_bind_obj(runtime_handler* obj, lua_State* state);

      static void _hookcb_static(lua_State* state, void* cbdata);

    public:
      runtime_handler(lua_State* state);
      runtime_handler(const std::string& lua_path);

      ~runtime_handler();

      static runtime_handler* get_attached_object(lua_State* state);

      lua::func_db* get_function_database();
      lua_State* get_lua_state();

      lua::debug::hook_handler* get_hook_handler();
      lua::debug::execution_flow* get_execution_flow();

      void stop_execution();
      bool run_execution(execution_context cb, void* cbdata);
      bool is_currently_executing();

      void set_logger(I_logger* logger) override;
  };
}

#endif