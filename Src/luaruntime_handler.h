#ifndef LUARUNTIME_HANDLER_HEADER
#define LUARUNTIME_HANDLER_HEADER

#include "defines.h"
#include "dllutil.h"
#include "I_debug_user.h"
#include "I_logger.h"
#include "luadebug_hook_handler.h"
#include "luadebug_execution_flow.h"
#include "luaincludes.h"
#include "lualibrary_loader.h"
#include "luavariant.h"
#include "library_linking.h"
#include "macro_helper.h"

#include "map"
#include "set"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif


namespace lua{
  class I_thread_handle;
  class I_thread_control;
  class I_thread_handle_reference;
  class thread_control;

  // [Thread-Safe]
  class I_runtime_handler: public I_debug_user{
    public:
      virtual ~I_runtime_handler(){}

      virtual void* get_lua_state_interface() const = 0;
      virtual lua::api::core get_lua_core_copy() const = 0;

      // deprecated
      // virtual lua::I_func_db* get_function_database_interface() = 0;
      
      virtual lua::I_thread_control* get_thread_control_interface() const = 0;
      virtual lua::I_library_loader* get_library_loader_interface() const = 0;

      // This creates a new thread for running the current file.
      // Return false if already running.
      virtual bool run_code() = 0;
      virtual bool currently_running() const = 0;
      virtual bool stop_running() = 0;
      virtual unsigned long get_main_thread_id() const = 0;

      // Only valid until loading another file or runtime_handler is deleted.
      virtual const lua::I_function_var* get_main_function() const = 0;

      // The function will skip load anothre file if currently running.
      // Returns -1 if something went wrong.
      virtual int load_file(const char* lua_path) = 0;
      virtual void load_std_libs() = 0;

      // can be NULL if no error from the start of the lua_State
      // can also be used for knowing if the object (runtime_handler) has any error when being created or not
      virtual const lua::I_variant* get_last_error_object() const = 0;
      virtual void reset_last_error() = 0;
  };


#ifdef LUA_CODE_EXISTS

  // NOTE: lua_State shouldn't be destroyed in this class' lifetime
  class runtime_handler: public I_runtime_handler{
    private:
      I_logger* _logger;
      lua_State* _state;

      lua::function_var* _main_func;

#if (_WIN64) || (_WIN32)
      CRITICAL_SECTION _obj_mutex;
#endif

      lua::thread_control* _thread_control = NULL;
      lua::library_loader* _library_loader = NULL;

      lua::variant* _last_err_obj = NULL;

      unsigned long _current_tid;
      bool _current_tid_valid = false;

      void _initiate_constructor();
      void _initiate_class();

      void _lock_object();
      void _unlock_object();

      // get error object and pops the value from the stack
      void _read_error_obj();
      void _set_error_obj(const I_variant* err_data = NULL);

      static void _set_bind_obj(runtime_handler* obj, lua_State* state);

    public:
      runtime_handler(bool load_library = true);
      runtime_handler(const std::string& lua_path, bool load_library = true);

      ~runtime_handler();

      static runtime_handler* get_attached_object(lua_State* state);

      lua_State* get_lua_state() const;
      void* get_lua_state_interface() const override;
      lua::api::core get_lua_core_copy() const override;

      lua::thread_control* get_thread_control() const;
      lua::library_loader* get_library_loader() const;

      lua::I_thread_control* get_thread_control_interface() const override;
      lua::I_library_loader* get_library_loader_interface() const override;

      bool run_code() override;
      bool currently_running() const override;
      bool stop_running() override;
      unsigned long get_main_thread_id() const override;

      const lua::I_function_var* get_main_function() const override; 

      int load_file(const char* lua_path) override;
      void load_std_libs() override;

      const lua::I_variant* get_last_error_object() const override;
      void reset_last_error() override;

      // NOTE: This will bind the logger to all tool membters in this object
      void set_logger(I_logger* logger) override;
  };

#endif // LUA_CODE_EXISTS

}


#define CPPLUA_CREATE_RUNTIME_HANDLER cpplua_create_runtime_handler
#define CPPLUA_CREATE_RUNTIME_HANDLER_STR MACRO_TO_STR_EXP(CPPLUA_CREATE_RUNTIME_HANDLER)

#define CPPLUA_DELETE_RUNTIME_HANDLER cpplua_delete_runtime_handler
#define CPPLUA_DELETE_RUNTIME_HANDLER_STR MACRO_TO_STR_EXP(CPPLUA_DELETE_RUNTIME_HANDLER)

// if lua_path is NULL, then the runtime_handler should only create empty lua_State
typedef lua::I_runtime_handler* (__stdcall *rh_create_func)(const char* lua_path, bool immediate_run, bool load_library);
typedef void (__stdcall *rh_delete_func)(lua::I_runtime_handler* handler);

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::I_runtime_handler* CPPLUA_CREATE_RUNTIME_HANDLER(const char* lua_path, bool load_library);
DLLEXPORT void CPPLUA_DELETE_RUNTIME_HANDLER(lua::I_runtime_handler* handler);
#endif // LUA_CODE_EXISTS

#endif