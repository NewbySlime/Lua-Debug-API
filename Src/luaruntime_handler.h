#ifndef LUARUNTIME_HANDLER_HEADER
#define LUARUNTIME_HANDLER_HEADER

#include "I_debug_user.h"
#include "I_logger.h"
#include "lua_includes.h"
#include "luadebug_hookhandler.h"
#include "luadebug_executionflow.h"
#include "luafunction_database.h"
#include "lualib_loader.h"
#include "library_linking.h"
#include "macro_helper.h"

#include "map"
#include "set"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif


namespace lua{
  class I_runtime_handler: public I_debug_user{
    public:
      typedef void (*execution_context)(void* data);

      virtual ~I_runtime_handler(){}

      virtual void* get_lua_state_interface() = 0;

      virtual lua::I_func_db* get_function_database_interface() = 0;
      virtual lua::debug::I_hook_handler* get_hook_handler_interface() = 0;
      virtual lua::debug::I_execution_flow* get_execution_flow_interface() = 0;
      virtual lua::I_lib_loader* get_library_loader_interface() = 0;

      virtual void stop_execution() = 0;
      virtual bool run_execution(execution_context cb, void* cbdata) = 0;
      virtual bool is_currently_executing() const = 0;

      virtual int run_current_file() = 0;
      virtual int load_file(const char* lua_path) = 0;
      virtual void load_std_libs() = 0;

#if (_WIN64) || (_WIN32)
      virtual DWORD get_running_thread_id() = 0;

      virtual void register_event_execution_finished(HANDLE event) = 0;
      virtual void remove_event_execution_finished(HANDLE event) = 0;
#endif

      // can be NULL if no error from the start of the lua_State
      // can also be used for knowing if the object (runtime_handler) has any error when being created or not
      virtual const lua::I_variant* get_last_error_object() const = 0;
      virtual void reset_last_error() = 0;
  };


  // NOTE: lua_State shouldn't be destroyed in this class' lifetime
  class runtime_handler: public I_runtime_handler{
    private:
      I_logger* _logger;

      lua_State* _state;

      bool _stop_thread = false;

      bool _create_own_lua_state = false;

#if (_WIN64) || (_WIN32)
      struct _t_entry_point_data{
        execution_context cb;
        void* cbdata;

        runtime_handler* _this;
      };

      // the handle shouldn't be removed by itself (thread), it should be removed by another thread (maybe calling thread), see stop_execution
      HANDLE _thread_handle = NULL;
      _t_entry_point_data* _thread_data = NULL;

      std::set<HANDLE> _event_finished;

      void _deinit_thread();

      static DWORD __stdcall _thread_entry_point(LPVOID data);
#endif

      lua::debug::hook_handler* _hook_handler = NULL;
      lua::debug::execution_flow* _execution_flow = NULL;

      lua::func_db* _function_database = NULL;

      lua::lib_loader* _library_loader = NULL;

      lua::variant* _last_err_obj = NULL;


      void _initiate_constructor();
      void _initiate_class();

      // get error object and pops the value from the stack
      void _read_error_obj();
      void _set_error_obj(const I_variant* err_data = NULL);
      
      void _hookcb();

      static void _set_bind_obj(runtime_handler* obj, lua_State* state);

      static void _hookcb_static(lua_State* state, void* cbdata);

    public:
      runtime_handler(lua_State* state);

      runtime_handler(bool load_library = true);
      runtime_handler(const std::string& lua_path, bool immediate_run = true, bool load_library = true);

      ~runtime_handler();

      static runtime_handler* get_attached_object(lua_State* state);

      lua_State* get_lua_state();
      void* get_lua_state_interface() override;

      lua::func_db* get_function_database();
      lua::debug::hook_handler* get_hook_handler();
      lua::debug::execution_flow* get_execution_flow();
      lua::lib_loader* get_library_loader();

      lua::I_func_db* get_function_database_interface() override;
      lua::debug::I_hook_handler* get_hook_handler_interface() override;
      lua::debug::I_execution_flow* get_execution_flow_interface() override;
      lua::I_lib_loader* get_library_loader_interface() override;

      void stop_execution() override;
      bool run_execution(execution_context cb, void* cbdata) override;
      bool is_currently_executing() const override;

      int run_current_file() override;
      int load_file(const char* lua_path) override;
      void load_std_libs() override;

#if (_WIN64) || (_WIN32)
      DWORD get_running_thread_id() override;

      void register_event_execution_finished(HANDLE event) override;
      void remove_event_execution_finished(HANDLE event) override;
#endif

      const lua::I_variant* get_last_error_object() const override;
      void reset_last_error() override;

      // NOTE: This will bind the logger to all tool membters in this object
      void set_logger(I_logger* logger) override;
  };
}


#define CPPLUA_CREATE_RUNTIME_HANDLER cpplua_create_runtime_handler
#define CPPLUA_CREATE_RUNTIME_HANDLER_STR MACRO_TO_STR_EXP(CPPLUA_CREATE_RUNTIME_HANDLER)

#define CPPLUA_DELETE_RUNTIME_HANDLER cpplua_delete_runtime_handler
#define CPPLUA_DELETE_RUNTIME_HANDLER_STR MACRO_TO_STR_EXP(CPPLUA_DELETE_RUNTIME_HANDLER)

// if lua_path is NULL, then the runtime_handler should only create empty lua_State
typedef lua::I_runtime_handler* (__stdcall *rh_create_func)(const char* lua_path, bool immediate_run, bool load_library);
typedef void (__stdcall *rh_delete_func)(lua::I_runtime_handler* handler);

#endif