#ifndef LUADEBUG_VARIABLE_WATCHER_HEADER
#define LUADEBUG_VARIABLE_WATCHER_HEADER

#include "I_debug_user.h"
#include "luaincludes.h"
#include "luavariant.h"
#include "macro_helper.h"

#include "set"
#include "vector"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif


// NOTE: since the introduction of thread dependent state system, every usage of the bound state will always use that state (of a thread) regardless which thread are calling it. It works by locking a state and then disabling the system for a moment.


namespace lua::debug{
  // [Not Thread-Safe]
  class I_variable_watcher{
    public:
      virtual ~I_variable_watcher(){};

      virtual bool fetch_global_table_data() = 0;
      virtual void update_global_table_ignore() = 0;
      virtual void clear_global_table_ignore() = 0;

      // defaults to true
      virtual void ignore_internal_variables(bool flag) = 0;
      virtual bool is_ignore_internal_variables() const = 0;

      virtual bool fetch_current_function_variables() = 0;

      virtual int get_variable_count() const = 0;
      virtual const char* get_variable_name(int idx) const = 0;
      // This function will create a copy
      virtual I_variant* get_variable(int idx) const = 0;
      // This function will create a copy
      virtual I_variant* get_variable(const char* var_name) const = 0;
      virtual int get_variable_type(int idx) const = 0;
      virtual int get_variable_type(const char* var_name) const = 0;
  };


#ifdef LUA_CODE_EXISTS

  class variable_watcher: public I_variable_watcher, public I_debug_user{
    private:
      struct _variable_data{
        public:
          std::string var_name;
          variant* var_data;

          int lua_type;
      };

      I_logger* _logger;

      lua_State* _state;

      std::vector<_variable_data*> _vdata_list;
      std::map<std::string, _variable_data*> _vdata_map;

      std::set<lua::comparison_variant> _global_ignore_variables;

      bool _ignore_internal_variables = true;

#if (_WIN64) || (_WIN32)
      CRITICAL_SECTION _object_mutex;
      CRITICAL_SECTION* _object_mutex_ptr;
#endif


      void _lock_object() const;
      void _unlock_object() const;

      // Also lock this object
      void _lock_state() const;
      // Also unlock this object
      void _unlock_state() const;

      void _clear_variable_data();
      

    public:
      variable_watcher(lua_State* state);
      ~variable_watcher();

      bool fetch_global_table_data() override;
      void update_global_table_ignore() override;
      void clear_global_table_ignore() override;

      void ignore_internal_variables(bool flag) override;
      bool is_ignore_internal_variables() const override;

      bool fetch_current_function_variables() override;

      int get_variable_count() const override;
      
      const char* get_variable_name(int idx) const override;

      lua::I_variant* get_variable(int idx) const override;
      lua::I_variant* get_variable(const char* name) const override;

      int get_variable_type(int idx) const override;
      int get_variable_type(const char* name) const override;

      void set_logger(I_logger* logger);
  };

#endif // LUA_CODE_EXISTS

}


#define CPPLUA_CREATE_VARIABLE_WATCHER cpplua_create_variable_watcher
#define CPPLUA_CREATE_VARIABLE_WATCHER_STR MACRO_TO_STR_EXP(CPPLUA_CREATE_VARIABLE_WATCHER)

#define CPPLUA_DELETE_VARIABLE_WATCHER cpplua_delete_variable_watcher
#define CPPLUA_DELETE_VARIABLE_WATCHER_STR MACRO_TO_STR_EXP(CPPLUA_DELETE_VARIABLE_WATCHER)

typedef lua::debug::I_variable_watcher* (__stdcall *vw_create_func)(void* istate);
typedef void (__stdcall *vw_delete_func)(lua::debug::I_variable_watcher* watcher);

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::debug::I_variable_watcher* CPPLUA_CREATE_VARIABLE_WATCHER(void* istate);
DLLEXPORT void CPPLUA_DELETE_VARIABLE_WATCHER(lua::debug::I_variable_watcher* watcher);
#endif // LUA_CODE_EXISTS

#endif