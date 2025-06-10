#ifndef LUADEBUG_VARIABLE_WATCHER_HEADER
#define LUADEBUG_VARIABLE_WATCHER_HEADER

#include "defines.h"
#include "I_debug_user.h"
#include "luaincludes.h"
#include "luavariant.h"
#include "macro_helper.h"

#include "mutex"
#include "set"
#include "vector"


// NOTE: since the introduction of thread dependent state system, every usage of the bound state will always be that state (of a thread) regardless which thread are calling it. It works by locking a state and then disabling the system for a moment.


namespace lua::debug{
  // [Not Thread-Safe]
  class I_variable_watcher{
    public:
      virtual ~I_variable_watcher(){};

      virtual void lock_object() const = 0;
      virtual void unlock_object() const = 0;

      virtual void update_global_ignore() = 0;
      virtual void clear_global_ignore() = 0;

      // NOTE: Update will replace all stored variable data with global variables.
      virtual bool update_global_variables() = 0;
      // NOTE: Update will replace all stored variable data with local variables.
      // This function will update the mapping of indexes of local variables, but will replaced by the next invocation of this function.
      virtual bool update_local_variables(int stack_idx = 0) = 0;

      // defaults to true
      virtual void ignore_internal_variables(bool flag) = 0;
      virtual bool is_ignore_internal_variables() const = 0;

      virtual bool is_internal_variables(const lua::I_variant* key) const = 0;

      // All getter function uses stored variable data updated using update functions.

      virtual int get_variable_count() const = 0;
      virtual const lua::I_variant* get_variable_key(int idx) const = 0;
      virtual lua::I_variant* get_variable_value_mutable(int idx) = 0;
      virtual lua::I_variant* get_variable_value_mutable(const lua::I_variant* key) = 0;
      virtual const lua::I_variant* get_variable_value(int idx) const = 0;
      virtual const lua::I_variant* get_variable_value(const lua::I_variant* key) const = 0;
      virtual int get_variable_type(int idx) const = 0;
      virtual int get_variable_type(const lua::I_variant* key) const = 0;

      virtual bool set_global_variable(const lua::I_variant* key, const lua::I_variant* value) = 0;
      // NOTE: stack idx of the local stack (function) is based on the update function for local variable.
      virtual bool set_local_variable(const lua::I_variant* key, const lua::I_variant* value) = 0;

      // Returns -1 if not in local mode.
      virtual int get_current_local_stack_idx() const = 0;

      virtual const lua::I_error_var* get_last_error() const = 0;
  };


#ifdef LUA_CODE_EXISTS

  class variable_watcher: public I_variable_watcher, public I_debug_user{
    private:
      struct _variable_data{
        public:
          lua::variant* var_key;
          lua::variant* var_data;

          int lua_type;
      };

      struct _local_variable_data{
        public:
          int local_idx;
      };

  
      I_logger* _logger;

      lua_State* _state;

      std::vector<_variable_data*> _vdata_list;
      std::map<lua::comparison_variant, _variable_data*> _vdata_map;
      std::set<lua::comparison_variant> _global_ignore_variables;

      std::map<lua::comparison_variant, _local_variable_data*> _vlocaldata_map;
      int _current_local_idx = -1;

      bool _ignore_internal_variables = true;

      lua::error_var* _current_error = NULL;

      std::recursive_mutex _object_mutex;
      std::recursive_mutex* _object_mutex_ptr = NULL;


      void _lock_object() const;
      void _unlock_object() const;

      // Also lock this object
      void _lock_state() const;
      // Also unlock this object
      void _unlock_state() const;

      lua::I_variant* _get_variable_value(int idx) const;
      lua::I_variant* _get_variable_value(const lua::I_variant* key) const;

      void _set_last_error(const lua::I_variant* err_obj, int err_code);
      void _clear_last_error();

      void _clear_variable_data();
      void _clear_local_variable_data();
      void _clear_object();

    public:
      variable_watcher(lua_State* state);
      ~variable_watcher();

      void lock_object() const override;
      void unlock_object() const override;

      void update_global_ignore() override;
      void clear_global_ignore() override;

      bool update_global_variables() override;
      bool update_local_variables(int stack_idx = 0) override;
      
      void ignore_internal_variables(bool flag) override;
      bool is_ignore_internal_variables() const override;

      bool is_internal_variables(const lua::I_variant* key) const override;

      int get_variable_count() const override;
      const lua::I_variant* get_variable_key(int idx) const override;
      lua::I_variant* get_variable_value_mutable(int idx) override;
      lua::I_variant* get_variable_value_mutable(const lua::I_variant* name) override;
      const lua::I_variant* get_variable_value(int idx) const override;
      const lua::I_variant* get_variable_value(const lua::I_variant* name) const override;
      int get_variable_type(int idx) const override;
      int get_variable_type(const lua::I_variant* name) const override;

      bool set_global_variable(const lua::I_variant* key, const lua::I_variant* value) override;
      bool set_local_variable(const lua::I_variant* key, const lua::I_variant* value) override;

      int get_current_local_stack_idx() const override;

      const lua::I_error_var* get_last_error() const override;

      void set_logger(I_logger* logger);
  };

#endif // LUA_CODE_EXISTS

}


#if (__linux)
// Just to make the compiler happy
#define __stdcall 
#endif


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