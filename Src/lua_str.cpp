#include "lua_str.h"
#include "luatable_util.h"
#include "strutil.h"

#include "set"


struct _self_reference_data{
  std::set<const void*> _referenced_table;
};

struct _ts_func_data{
  int indent_offset;
  std::string result_str;

  _self_reference_data* _sr_data;
};


std::string _to_string(lua_State* state, int state_idx, _self_reference_data* _sr_data, int indent_offset);
void _ts_table_iter_cb(lua_State* state, int key_idx, int value_idx, int iter_idx, void* cb_data){
  _ts_func_data* _fdata = (_ts_func_data*)cb_data;

  int _current_indent = _fdata->indent_offset+1;
  
  int _key_type = lua_type(state, key_idx);
  std::string _key_name = _to_string(state, key_idx, _fdata->_sr_data, 0);
  std::string _value_str = ""; 
  _value_str = _to_string(state, value_idx, _fdata->_sr_data, _current_indent);

  std::string _indent_offsetter = std::string(_current_indent, '\t');
  _fdata->result_str +=
    ((_fdata->result_str.length() > 0)? (",\n" + _indent_offsetter): "") +
    format_str("%s: %s", _key_name.c_str(), _value_str.c_str())
  ;
}


std::string _to_string(lua_State* state, int state_idx, _self_reference_data* _sr_data, int indent_offset = 0){
  std::string _result = "";

  switch(lua_type(state, state_idx)){
    break; case LUA_TNONE:{
      _result = "none";
    }

    break; case LUA_TNIL:{
      _result = "null";
    }

    break; case LUA_TLIGHTUSERDATA:{
      _result = format_str("lightuserdata 0x%X", lua_touserdata(state, state_idx));
    }

    break; case LUA_TNUMBER:{
      lua_Number _number = lua_tonumber(state, state_idx);
      _result = format_str("%f", _number);
    }

    break; case LUA_TSTRING:{
      const char* _cstr = lua_tolstring(state, state_idx, NULL);
      if(!_cstr){
        _result = "nullstr";
        break;
      } 

      _result = _cstr;
    }

    break; case LUA_TTABLE:{
      if(_sr_data->_referenced_table.find(lua_topointer(state, state_idx)) != _sr_data->_referenced_table.end())
        break;

      int _current_indent = indent_offset-1;
      int _indent_next_off = _current_indent+1;
      
      _ts_func_data _fdata;{
        _fdata.indent_offset = _indent_next_off > 0? _indent_next_off: 0;
        _fdata.result_str = "";

        _fdata._sr_data = _sr_data;
      }

      try{
        _sr_data->_referenced_table.insert(lua_topointer(state, state_idx));
        lua::iterate_table(state, state_idx, _ts_table_iter_cb, &_fdata);
      }
      catch(std::exception* e){
        _result = e->what();
        
        delete e;
      }

      _current_indent += 1;
      _indent_next_off += 1;
      
      std::string _indent_next = _indent_next_off > 0? std::string(_indent_next_off, '\t'): "";
      std::string _indent = _current_indent > 0? std::string(_current_indent, '\t'): "";
      
      _result = format_str("{\n%s%s\n%s}", _indent_next.c_str(), _fdata.result_str.c_str(), _indent.c_str());
    }

    break; case LUA_TFUNCTION:{
      _result = "function";
    }

    break; case LUA_TUSERDATA:{
      _result = format_str("userdata 0x%X", lua_touserdata(state, state_idx));
    }

    break; case LUA_TTHREAD:{
      _result = "thread";
    }

    break; default:{
      _result = "undefined type";
    }
  }

  return _result;
}


std::string lua::to_string(lua_State* state, int state_idx){
  _self_reference_data _data;
  return _to_string(state, state_idx, &_data);
}