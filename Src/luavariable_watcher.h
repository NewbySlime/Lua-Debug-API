#ifndef LUAVARIANT_WATCHER_HEADER
#define LUAVARIANT_WATCHER_HEADER

#include "I_debug_user.h"
#include "lua_includes.h"
#include "lua_variant.h"
#include "macro_helper.h"

#include "set"
#include "vector"


namespace lua::debug{
  class I_variable_watcher{
    public:
      virtual ~I_variable_watcher(){};

      virtual bool fetch_global_table_data() = 0;
      virtual void update_global_table_ignore() = 0;

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

      void _fetch_function_variable_data(lua_Debug* debug_data);
      void _clear_variable_data();

      static void _set_bind_obj(variable_watcher* obj, lua_State* state);

    public:
      variable_watcher(lua_State* state);
      ~variable_watcher();

      static variable_watcher* get_attached_object(lua_State* state);

      bool fetch_global_table_data() override;
      void update_global_table_ignore() override;

      bool fetch_current_function_variables() override;

      int get_variable_count() const override;
      
      const char* get_variable_name(int idx) const override;

      lua::I_variant* get_variable(int idx) const override;
      lua::I_variant* get_variable(const char* name) const override;

      int get_variable_type(int idx) const override;
      int get_variable_type(const char* name) const override;

      void set_logger(I_logger* logger);
  };
}


#define CPPLUA_CREATE_VARIABLE_WATCHER cpplua_create_variable_watcher
#define CPPLUA_CREATE_VARIABLE_WATCHER_STR MACRO_TO_STR_EXP(CPPLUA_CREATE_VARIABLE_WATCHER)

#define CPPLUA_DELETE_VARIABLE_WATCHER cpplua_delete_variable_watcher
#define CPPLUA_DELETE_VARIABLE_WATCHER_STR MACRO_TO_STR_EXP(CPPLUA_DELETE_VARIABLE_WATCHER)

typedef lua::debug::I_variable_watcher* (__stdcall *vw_create_func)(void* interface_state);
typedef void (__stdcall *vw_delete_func)(lua::debug::I_variable_watcher* watcher);

#endif