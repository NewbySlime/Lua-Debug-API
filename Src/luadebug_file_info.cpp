#include "error_util.h"
#include "luaapi_compilation_context.h"
#include "luaapi_state.h"
#include "luadebug_file_info.h"
#include "luamemory_util.h"
#include "luautility.h"
#include "luavariant_util.h"
#include "memdynamic_management.h"

#include "algorithm"

#if (_WIN64) | (_WIN32)
#include "Windows.h"
#endif

// This is used for creating a dummy line at the start of portion of Lua code.
// Reason for this approach is to mitigate automatically validated line when that portion of the Code is empty.
#define LUA_EMPTY_CODE_FILLER "\r\n"
#define LUA_EMPTY_CODE_FILLER_SIZE (sizeof(LUA_EMPTY_CODE_FILLER)-1)


using namespace error::util;
using namespace lua;
using namespace lua::api;
using namespace lua::debug;
using namespace lua::memory;
using namespace lua::utility;
using namespace ::memory;


#ifdef LUA_CODE_EXISTS

static const I_dynamic_management* __dm = get_memory_manager();


// make sure that the buffer is a null terminated string
static bool _has_newline(const char* buffer, size_t idx, size_t* next_idx){
  bool _result = false;
  switch(buffer[idx]){
    break; case '\n':
      idx += 1;
      _result = true;

    break; case '\r':
      idx += buffer[idx+1] == '\n'? 2: 1;
      _result = true;
  }

  *next_idx = idx;
  return _result;
}


file_info::file_info(const char* file_path){
  _lstate = luaL_newstate();

  set_file_path(file_path);
  refresh_info();
}

file_info::~file_info(){
  _delete_error_data();

  lua_close(_lstate);
}


void file_info::_set_error_data(long long errcode, const char* error_str){
  _delete_error_data();

  string_var _str = error_str;
  _err_data = __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_str, errcode);
}

void file_info::_copy_error_data(){
  _delete_error_data();

  const core _lc = as_lua_api_core(_lstate);
  _err_data = __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_lc, -1);
}

void file_info::_delete_error_data(){
  if(_err_data == NULL)
    return;

  __dm->delete_class_dbg<error_var>(_err_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  _err_data = NULL;
}


void file_info::_update_file_info(char* buffer){
  _lines_info.reserve(_lines_info.size());
  _lines_info.resize(0);

  int _line_count = 1;
  line_info __linfo;
    __linfo.buffer_idx_start = 0;
    __linfo.valid_line = false;
  _lines_info.insert(_lines_info.end(), __linfo);

  size_t i = LUA_EMPTY_CODE_FILLER_SIZE;
  while(buffer[i] > 0){
    if(_has_newline(buffer, i, &i)){
      line_info _linfo;
        _linfo.buffer_idx_start = i-LUA_EMPTY_CODE_FILLER_SIZE;
        _linfo.valid_line = false;
      _lines_info.insert(_lines_info.end(), _linfo);

      _line_count++;
      i--;
    }

    i++;
  }

  _check_code_buffer(buffer, i, 0, _line_count);
}


bool file_info::_check_code_buffer(char* buffer, size_t buffer_size, long start_line, long finish_line, long recur_idx){
  if((finish_line-start_line) <= 0)
    return false;

  long _start_line_idx = _lines_info[start_line].buffer_idx_start+LUA_EMPTY_CODE_FILLER_SIZE;
  long _last_line_idx = buffer_size;
  if(finish_line < _lines_info.size())
    _last_line_idx = _lines_info[finish_line].buffer_idx_start+LUA_EMPTY_CODE_FILLER_SIZE;
  
  char _prev_last_char = buffer[_last_line_idx];
  char _tmp_buffer[LUA_EMPTY_CODE_FILLER_SIZE];

  buffer[_last_line_idx] = '\0';
  memcpy(_tmp_buffer, &buffer[_start_line_idx-LUA_EMPTY_CODE_FILLER_SIZE], LUA_EMPTY_CODE_FILLER_SIZE);
  memcpy(&buffer[_start_line_idx-LUA_EMPTY_CODE_FILLER_SIZE], LUA_EMPTY_CODE_FILLER, LUA_EMPTY_CODE_FILLER_SIZE);

  int _errcode = luaL_loadstring(_lstate, &buffer[_start_line_idx-LUA_EMPTY_CODE_FILLER_SIZE]);
  if(_errcode != LUA_OK){
    if(recur_idx < 1){
      variant* _err_obj = to_variant(_lstate, -1);
      string_store _str; _err_obj->to_string(&_str);
      cpplua_delete_variant(_err_obj);
      lua_pop(_lstate, 1); // pop error obj

      _set_error_data(_errcode, _str.data.c_str());
    }
    else{ // to mitigate problem with "local function" syntax not detected by Lua debug 
      lua_pop(_lstate, 1); // pop error obj
      if(_check_code_buffer(buffer, buffer_size, start_line+1, finish_line, recur_idx+1)){
        if(start_line < _lines_info.size())
          _lines_info[start_line].valid_line = true;
      }
    }

    return false;
  }

  buffer[_last_line_idx] = _prev_last_char;
  memcpy(&buffer[_start_line_idx-LUA_EMPTY_CODE_FILLER_SIZE], _tmp_buffer, LUA_EMPTY_CODE_FILLER_SIZE);

  lua_Debug _dbg;
  lua_getinfo(_lstate, ">L", &_dbg);

  const core _lc = as_lua_api_core(_lstate);
  table_var _tvar(&_lc, -1);
  const I_variant** _key_list = _tvar.get_keys();

  struct _check_data{
    long start_line;
    long finish_line;
  };

  std::vector<_check_data> _check_list;

  // put the valid lines in a vector, to be ordered
  std::vector<long> _valid_list;
  for(size_t i = 0; _key_list[i]; i++){
    const I_variant* _curr_key = _key_list[i];
    if(_curr_key->get_type() != I_number_var::get_static_lua_type())
      continue;

    long _curr_line = dynamic_cast<const I_number_var*>(_curr_key)->get_number();
    _valid_list.insert(_valid_list.end(), _curr_line);
  }

  std::sort(_valid_list.begin(), _valid_list.end());

  long _last_line = start_line;
  for(auto _curr_line: _valid_list){
    if(_curr_line <= 1)
      continue;

    _curr_line += start_line-2;
    _check_data _data;
      _data.start_line = _last_line;
      _data.finish_line = _curr_line;
    _check_list.insert(_check_list.end(), _data);

    _lines_info[_curr_line].valid_line = true;
    _last_line = _curr_line+1;
  }

  // table lines
  lua_pop(_lstate, 1);

  for(auto _iter: _check_list)
    _check_code_buffer(buffer, buffer_size, _iter.start_line, _iter.finish_line, recur_idx+1);

  return _check_list.size() > 0;
}


void file_info::set_file_path(const char* file_path){
  _file_path = file_path;
}

const char* file_info::get_file_path(){
  return _file_path.c_str();
}


void file_info::refresh_info(){
  if(_file_path.size() <= 0){
    _set_error_data(LUA_ERRFILE, "File path is invalid.");
    return;
  }

#if (_WIN64) | (_WIN32)
  HANDLE _file_handle = CreateFile(
    _file_path.c_str(),
    GENERIC_READ,
    0,
    NULL,
    OPEN_EXISTING,
    0,
    NULL
  );

  if(_file_handle == INVALID_HANDLE_VALUE){
    _set_error_data(LUA_ERRFILE, get_windows_error_message(GetLastError()).c_str());
    return;
  }

  DWORD _file_size = GetFileSize(_file_handle, NULL);
  char* _file_buffer = (char*)__dm->malloc(LUA_EMPTY_CODE_FILLER_SIZE+_file_size+1, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  
  DWORD _read_len; ReadFile(_file_handle, &_file_buffer[LUA_EMPTY_CODE_FILLER_SIZE], _file_size, &_read_len, NULL);
  _file_buffer[_read_len+LUA_EMPTY_CODE_FILLER_SIZE] = '\0';

  _update_file_info(_file_buffer);

  __dm->free(_file_buffer, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  CloseHandle(_file_handle);
#endif
}


long file_info::get_line_count(){
  return _lines_info.size();
}


bool file_info::is_line_valid(long line_idx){
  if(line_idx < 0 || line_idx >= _lines_info.size())
    return false;
  
  return _lines_info[line_idx].valid_line;
}

long file_info::get_line_idx(long line_idx){
  if(line_idx < 0 || line_idx >= _lines_info.size())
    return -1;

  return _lines_info[line_idx].buffer_idx_start;
}


const I_error_var* file_info::get_last_error(){
  return _err_data;
}


// MARK: DLL functions

DLLEXPORT I_file_info* CPPLUA_CREATE_FILE_INFO(const char* file_path){
  return __dm->new_class_dbg<file_info>(DYNAMIC_MANAGEMENT_DEBUG_DATA, file_path);
}

DLLEXPORT void CPPLUA_DELETE_FILE_INFO(I_file_info* obj){
  __dm->delete_class_dbg(obj, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}


#endif