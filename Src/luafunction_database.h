#ifndef LUAFUNCTION_DATABASE_HEADER
#define LUAFUNCTION_DATABASE_HEADER

#include "I_debug_user.h"
#include "library_linking.h"
#include "luaincludes.h"
#include "luavariant.h"
#include "luavariant_arr.h"
#include "luavariant_util.h"
#include "luadebug_hookhandler.h"
#include "macro_helper.h"
#include "string_util.h"

#include "map"
#include "set"
#include "thread"
#include "vector"


// This code will be statically bind to the compilation file
// If a code returns an interface (I_xx) create a copy with using statically linked compilation function if the code that returns comes from dynamic library

namespace lua{
  class I_func_db: public I_debug_user{
    public:
      typedef void (*function_cb)(const lua::I_vararr* args, lua::I_vararr* results);

      virtual ~I_func_db(){}

      virtual bool remove_lua_function_def(const char* function_name) = 0;
      virtual bool remove_c_function_def(const char* function_name) = 0;

      virtual bool expose_c_function_nonstrict(const char* function_name, function_cb cb) = 0;

      virtual bool call_lua_function_nonstrict(const char* function_name, const lua::I_vararr* args, lua::I_vararr* results) = 0;
  };


#ifdef LUA_CODE_EXISTS

  // NOTE: lua_State shouldn't be destroyed in this class' lifetime
  class func_db: public I_func_db{
    private:
      struct _lua_func_metadata{
        public:
          std::vector<int> _func_args_type;
          int _return_type;
      };

      struct _c_func_metadata{
        public:
          void* function_address;
      };

      template<int N_count> struct _args_list{
        private:
          lua::variant* _list[N_count];

        public:
          constexpr _args_list(){
            for(int i = 0; i < N_count; i++)
              _list[i] = NULL;
          }

          constexpr lua::variant* operator[](int n){
            return _list[n];
          }

          constexpr int get_count(){
            return N_count;
          }

          
          void set_idx(int idx, lua::variant* data){
            _list[idx] = data;
          }
      };



      I_logger* _current_logger;

      debug::hook_handler* _hook_handler;

      lua_State* _this_state;
      std::map<std::string, _lua_func_metadata*> _lua_metadata_map;
      std::map<std::string, _c_func_metadata*> _c_metadata_map;

      std::map<std::thread::id, std::string> _current_thread_funccall;

      _lua_func_metadata* _lua_create_metadata(const std::string& name);
      _lua_func_metadata* _lua_get_metadata(const std::string& name);
      bool _lua_delete_metadata(const std::string& name);

      _c_func_metadata* _c_create_metadata(const std::string& name);
      _c_func_metadata* _c_get_metadata(const std::string& name);
      bool _c_delete_metadata(const std::string& name);


      lua::variant* _call_lua_function(int arg_count, int return_count, int msgh = 0);

      static int _c_function_cb_nonstrict(lua_State* state);
      static void _hook_cb(lua_State* state, void* cbdata);


    public:
      func_db(lua_State* state);
      ~func_db();

      static func_db* get_state_db(lua_State* state);

      void set_logger(I_logger* logger) override;

      bool remove_lua_function_def(const char* function_name) override;
      bool remove_c_function_def(const char* function_name) override;

      bool expose_c_function_nonstrict(const char* function_name, function_cb cb) override;

      bool call_lua_function_nonstrict(const char* function_name, const lua::I_vararr* args, lua::I_vararr* results) override;


    private:
      template<int N_count, int N_idx, class T_result, typename T_cb, class... T_args> static T_result _call_c_function(_args_list<N_count>* _args_arr, T_cb cb, T_args... args){
        if constexpr(N_idx > 0)
          return _call_c_function<N_count, N_idx-1, T_result, T_cb, variant*, T_args...>(_args_arr, cb, _args_arr->operator[](N_idx), args...);
        else
          return cb(_args_arr->operator[](N_idx), args...);
      } 


    public:
      // strict arguments and result type
      template<typename var_result, typename... var_args> bool expose_lua_function(const char* function_name){
        if(!_this_state)
          return false;

        lua_getglobal(_this_state, function_name);
        if(lua_type(_this_state, -1) != LUA_TFUNCTION){
          if(_current_logger){
            _current_logger->print_error(format_str("Variable (%s) is not a function.\n", function_name).c_str());
          }

          goto on_error_label;
        }

        {_lua_func_metadata* _metadata = _lua_create_metadata(function_name);
          _metadata->_func_args_type.clear();

          _metadata->_return_type = var_result::get_static_lua_type();
          ([&]{
            _metadata->_func_args_type.insert(_metadata->_func_args_type.end(), var_args::get_static_lua_type());
          }(), ...);

          lua_pop(_this_state, 1);
          return true;
        }

        on_error_label:{
          lua_pop(_this_state, 1);
          _lua_delete_metadata(function_name);
        return false;}
      }

      // strict arguments and result type
      template<typename var_result, typename... var_args> bool expose_c_function(const char* function_name, var_result (*function_cb)(var_args...)){
        if(!_this_state)
          return false;

        auto _iter = _c_metadata_map.find(function_name);
        if(_iter != _c_metadata_map.end()){
          if(_current_logger){
            _current_logger->print_error(format_str("Function (%s) already exposed to lua.\n", function_name).c_str());
          }

          return false;
        }

        _c_func_metadata* _metadata = _c_create_metadata(function_name);
        _metadata->function_address = (void*)function_cb;

        lua_pushcclosure(_this_state, [](lua_State* state){
          func_db* _db = get_state_db(state);
          std::string _fname = _db->_current_thread_funccall[std::this_thread::get_id()];

          typedef var_result (*func_cb_type)(var_args...);
          _c_func_metadata* _metadata = _db->_c_get_metadata(_fname);
          var_result (*func_cb)(var_args...) = (func_cb_type)_metadata->function_address;

          const int _arg_size = sizeof...(var_args);
          _args_list<_arg_size> _list;
          
          // initialize args
          for(int i = 0; i < _arg_size; i++){
            int _stack_idx = -1-(_arg_size-i-1);

            _list.set_idx(i, lua::to_variant(state, _stack_idx));
          }

          var_result _result = _call_c_function<_arg_size, _arg_size-1, var_result>(&_list, func_cb);
          _result.push_to_stack(state);

          // deinit args
          for(int i = 0; i < _arg_size; i++)
            delete _list[i];

          return (int)(var_result::get_static_lua_type() != LUA_TNIL);
        }, 0);

        lua_setglobal(_this_state, function_name);
        return true;
      }


      // Always check the returned value, since there's a chance that the returned value is a error_var
      // NOTE: don't forget to delete returned variant if returned variant is not NULL
      template<typename... var_args> lua::variant* call_lua_function(const char* function_name, var_args... args){
        if(!_this_state)
          return NULL;
    
        auto _iter = _lua_metadata_map.find(function_name);
        if(_iter == _lua_metadata_map.end()){
          if(_current_logger){
            _current_logger->print_error(format_str("Cannot get Function metadata of (%s).", function_name).c_str());
          }

          goto on_error_label;
        }

        {lua_getglobal(_this_state, function_name);
          int _curr_idx = 0;
          ([&]{
            if(_current_logger){
              if(_curr_idx >= _iter->second->_func_args_type.size())
                _current_logger->print_warning(format_str("Function (%s) has less exposed argument than expected (%d/%d).\n",
                  function_name,
                  _curr_idx+1,
                  _iter->second->_func_args_type.size()
                ).c_str());
              else if(args->get_type() != _iter->second->_func_args_type[_curr_idx])
                _current_logger->print_warning(format_str("Function (%s), Argument #%d mismatched type. (%s:%s)\n",
                  function_name,
                  _curr_idx,
                  lua_typename(_this_state, args->get_type()),
                  lua_typename(_this_state, _iter->second->_func_args_type[_curr_idx])
                ).c_str());
            }

            args->push_to_stack(_this_state);
            _curr_idx++;
          }(), ...);

          return _call_lua_function(_curr_idx, _iter->second->_return_type != LUA_TNIL);
        }
        
        on_error_label:{
        return new lua::variant();}
      }
  };

#endif // LUA_CODE_EXISTS

}


#define CPPLUA_CREATE_FUNCTION_DATABASE cpplua_create_function_database
#define CPPLUA_CREATE_FUNCTION_DATABASE_STR MACRO_TO_STR_EXP(CPPLUA_CREATE_FUNCTION_DATABASE)

#define CPPLUA_DELETE_FUNCTION_DATABASE cpplua_delete_function_database
#define CPPLUA_DELETE_FUNCTION_DATABASE_STR MACRO_TO_STR_EXP(CPPLUA_DELETE_FUNCTION_DATABASE)

typedef lua::I_func_db* (__stdcall *fdb_create_func)(void* istate);
typedef void (__stdcall *fdb_delete_func)(lua::I_func_db* database);

#ifdef LUA_CODE_EXISTS
DLLEXPORT lua::I_func_db* CPPLUA_CREATE_FUNCTION_DATABASE(void* istate);
DLLEXPORT void CPPLUA_DELETE_FUNCTION_DATABASE(lua::I_func_db* database);
#endif // LUA_CODE_EXISTS

#endif