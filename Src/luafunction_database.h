#ifndef LUAFUNCTION_DATABASE_HEADER
#define LUAFUNCTION_DATABASE_HEADER

#include "I_debug_user.h"
#include "lua_includes.h"
#include "lua_str.h"
#include "lua_variant.h"
#include "luadebug_hookhandler.h"
#include "strutil.h"

#include "map"
#include "set"
#include "thread"
#include "vector"


namespace lua{
  class func_db: I_debug_user{
    private:
      struct _func_metadata{
        public:
          std::vector<int> _func_args_type;

          int _return_type;
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
      std::map<std::string, _func_metadata*> _lua_metadata_map;
      std::map<std::string, _func_metadata*> _c_metadata_map;

      std::map<std::thread::id, std::string> _current_thread_funccall;

      _func_metadata* _lua_create_metadata(const std::string& name);
      _func_metadata* _lua_get_metadata(const std::string& name);
      bool _lua_delete_metadata(const std::string& name);

      _func_metadata* _c_create_metadata(const std::string& name);
      _func_metadata* _c_get_metadata(const std::string& name);
      bool _c_delete_metadata(const std::string& name);


      variant* _call_function(int arg_count, int return_count, int msgh = 0);

      static void _hook_cb(lua_State* state, void* cbdata);


    public:
      func_db(lua_State* state);
      ~func_db();

      static func_db* get_state_db(lua_State* state);

      void set_logger(I_logger* logger) override;

      bool remove_lua_function_def(const char* function_name);



    private:
      template<int N_count, int N_idx, class T_result, typename T_cb, class... T_args> static T_result _call_c_function(_args_list<N_count>* _args_arr, T_cb cb, T_args... args){
        if constexpr(N_idx > 0)
          return _call_c_function<N_count, N_idx-1, T_result, T_cb, variant*, T_args...>(_args_arr, cb, _args_arr->operator[](N_idx), args...);
        else
          return cb(_args_arr->operator[](N_idx), args...);
      } 


    public:
      template<typename var_result, typename... var_args> bool expose_lua_function(const char* function_name){
        if(!_this_state)
          return false;

        lua_getglobal(_this_state, function_name);
        if(lua_type(_this_state, -1) != LUA_TFUNCTION){
          _current_logger->print_error(format_str("Variable (%s) is not a function.\n", function_name));
          goto on_error_label;
        }

        {_func_metadata* _metadata = _lua_create_metadata(function_name);
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

      template<typename var_result, typename... var_args> bool expose_c_function(const char* function_name, var_result (*function_cb)(var_args...)){
        if(!_this_state)
          return false;

        auto _iter = _c_metadata_map.find(function_name);
        if(_iter != _c_metadata_map.end()){
          _current_logger->print_error(format_str("Function (%s) already exposed to lua.\n", function_name));
          return false;
        }

        _func_metadata* _metadata = _c_create_metadata(function_name);
        _metadata->function_address = (void*)function_cb; 

        lua_pushcclosure(_this_state, [](lua_State* state){
          func_db* _db = get_state_db(state);
          std::string _fname = _db->_current_thread_funccall[std::this_thread::get_id()];

          typedef var_result (*func_cb_type)(var_args...);
          _func_metadata* _metadata = _db->_c_get_metadata(_fname);
          var_result (*func_cb)(var_args...) = (func_cb_type)_metadata->function_address;

          const int _arg_size = sizeof...(var_args);
          _args_list<_arg_size> _list;
          
          // initialize args
          for(int i = 0; i < _arg_size; i++){
            int _stack_idx = -1-(_arg_size-i-1);

            std::string _val_str = lua::to_string(state, _stack_idx);
            printf("arg i%d: %s\n", i, _val_str.c_str());

            _list.set_idx(i, to_variant(state, _stack_idx));
          }

          var_result _result = _call_c_function<_arg_size, _arg_size-1, var_result>(&_list, func_cb);
          _result.push_to_stack(state);

          // deinit args
          for(int i = 0; i < _arg_size; i++)
            delete _list[i];

          printf("function called: %s\n", _db->_current_thread_funccall[std::this_thread::get_id()].c_str());

          return (int)(var_result::get_static_lua_type() != LUA_TNIL);
        }, 0);

        lua_setglobal(_this_state, function_name);
        return true;
      }


      // Always check the returned value, since there's a chance that the returned value is a error_var
      // NOTE: don't forget to delete returned variant if returned variant is not NULL
      template<typename... var_args> lua::variant* call_lua_function(const char* function_name, var_args... args){
        auto _iter = _lua_metadata_map.find(function_name);
        if(_iter == _lua_metadata_map.end()){
          _current_logger->print_error(format_str("Cannot get Function metadata of (%s).", function_name));
          goto on_error_label;
        }

        {lua_getglobal(_this_state, function_name);
          int _curr_idx = 0;
          ([&]{
            if(_curr_idx >= _iter->second->_func_args_type.size())
              _current_logger->print_warning(format_str("Function (%s) has less exposed argument than expected (%d/%d).\n",
                function_name,
                _curr_idx+1,
                _iter->second->_func_args_type.size()
              ));
            else if(args->get_type() != _iter->second->_func_args_type[_curr_idx])
              _current_logger->print_warning(format_str("Function (%s), Argument #%d mismatched type. (%s:%s)\n",
                function_name,
                _curr_idx,
                lua_typename(_this_state, args->get_type()),
                lua_typename(_this_state, _iter->second->_func_args_type[_curr_idx])
              ));

            args->push_to_stack(_this_state);
            _curr_idx++;
          }(), ...);

          return _call_function(_curr_idx, _iter->second->_return_type != LUA_TNIL);
        }
        
        on_error_label:{
        return new lua::variant();}
      }
  };
}

#endif