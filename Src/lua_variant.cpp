#include "lua_str.h"
#include "lua_variant.h"
#include "luastack_iter.h"
#include "luatable_util.h"
#include "stdlogger.h"
#include "strutil.h"

#include "cmath"
#include "cstring"
#include "thread"
#include "set"

#define NUM_VAR_EQUAL_ROUND_COMP 0.01


using namespace lua;


stdlogger _default_logger = stdlogger();
const I_logger* _logger = &_default_logger;



// MARK: lua::set_default_logger
void lua::set_default_logger(I_logger* logger){
  _logger = logger;
}




// MARK: lua::variant def
variant::variant(){}


int variant::get_type() const{
  return LUA_TNIL;
}


int variant::compare_with(const variant* rhs) const{
  int _lhs_type = this->get_type();
  int _rhs_type = rhs->get_type();
  if(_lhs_type != _rhs_type)
    return _lhs_type < _rhs_type? -1: 1;

  return this->_compare_with(rhs);
}


void variant::setglobal(lua_State* state, std::string var_name){
  setglobal(state, var_name.c_str());
}

void variant::setglobal(lua_State* state, const char* var_name){
  this->push_to_stack(state);
  lua_setglobal(state, var_name);
}




// MARK: lua::string_var def
string_var::string_var(){
  _init_class();
}

string_var::string_var(const string_var& var1){
  _init_class();
  _set_to_cstr(var1._str_mem, var1._mem_size);
}

string_var::string_var(const char* cstr){
  _init_class();

  size_t _str_len = std::strlen(cstr);
  _set_to_cstr(cstr, _str_len);
}

string_var::string_var(const char* cstr, size_t len){
  _init_class();
  _set_to_cstr(cstr, len);
}

string_var::string_var(const std::string& str){
  _init_class();
  _set_to_cstr(str.c_str(), str.size());
}

string_var::string_var(lua_State* state, int stack_idx){
  _init_class();
  from_state(state, stack_idx);
}

string_var::~string_var(){
  free(_str_mem);
}


void string_var::_init_class(){
  _mem_size = 0;
  _str_mem = (char*)malloc(_mem_size+1);

  _put_delimiter_at_end();
}


void string_var::_put_delimiter_at_end(){
  _str_mem[_mem_size] = '\0';
}


void string_var::_resize_mem(size_t newlen){
  _str_mem = (char*)realloc(_str_mem, newlen+1);
  _mem_size = newlen;
}


void string_var::_set_to_cstr(const char* cstr, size_t strlen){
  _resize_mem(strlen);

  memcpy(_str_mem, cstr, strlen);
  _put_delimiter_at_end();
}

void string_var::_append_cstr(const char* cstr, size_t strlen){
  size_t _original_len = _mem_size;
  _resize_mem(_mem_size + strlen);

  char* _offset_str_mem = (char*)((long long)(_str_mem + _original_len));
  memcpy(_offset_str_mem, cstr, strlen);
  _put_delimiter_at_end();
}


int string_var::_strcmp(const string_var& var1, const string_var& var2){
  size_t _check_len = std::min(var1._mem_size, var2._mem_size);
  for(int i = 0; i < _check_len; i++){
    char _v1_ch = var1[i];
    char _v2_ch = var2[i];

    if(_v1_ch < _v2_ch)
      return -1;
    else if(_v1_ch > _v2_ch)
      return 1;
  }

  if(var1._mem_size < var2._mem_size)
    return -1;
  else if(var1._mem_size > var2._mem_size)
    return 1;
  else
    return 0;
}


int string_var::_compare_with(const variant* rhs) const{
  return _strcmp(*this, *(string_var*)rhs);
}


int string_var::get_type() const{
  return LUA_TSTRING;
}


bool string_var::from_state(lua_State* state, int stack_idx){
  int _type = lua_type(state, stack_idx);
  switch(_type){
    break;
      case LUA_TSTRING:
      case LUA_TNUMBER:
    
    break; default:{
      _logger->print_error(format_str("Mismatched type when converting at stack (%d). (%s-%s)\n",
        stack_idx,
        lua_typename(state, _type),
        lua_typename(state, get_type())
      ));

      return false;
    }
  }

  size_t _str_len;
  const char* _cstr = lua_tolstring(state, stack_idx, &_str_len);
  _set_to_cstr(_cstr, _str_len);

  return true;
}

void string_var::push_to_stack(lua_State* state) const{
  lua_pushlstring(state, _str_mem, _mem_size);
}

variant* string_var::create_copy() const{
  return new string_var(_str_mem, _mem_size);
}


std::string string_var::to_string() const{
  return std::string(_str_mem, _mem_size);
}


string_var& string_var::operator+=(const string_var& var1){
  append(var1);
  return *this;
}

string_var& string_var::operator+=(const std::string& var1){
  append(var1);
  return *this;
}

string_var& string_var::operator+=(const char* var1){
  append(var1);
  return *this;
}


string_var string_var::operator+(const string_var& var1) const{
  string_var _result = *this;
  _result.append(var1);

  return _result;
}

string_var string_var::operator+(const std::string& var1) const{
  string_var _result = *this;
  _result.append(var1);

  return _result;
}

string_var string_var::operator+(const char* var1) const{
  string_var _result = *this;
  _result.append(var1);

  return _result;
}


const char& string_var::operator[](size_t i) const{
  return _str_mem[i];
}


bool string_var::operator<(const string_var& var1) const{
  int _str_res = _strcmp(*this, var1);
  return _str_res < 0;
}

bool string_var::operator>(const string_var& var1) const{
  int _str_res = _strcmp(*this, var1);
  return _str_res > 0;
}

bool string_var::operator<=(const string_var& var1) const{
  int _str_res = _strcmp(*this, var1);
  return _str_res <= 0;
}

bool string_var::operator>=(const string_var& var1) const{
  int _str_res = _strcmp(*this, var1);
  return _str_res >= 0;
}

bool string_var::operator==(const string_var& var1) const{
  int _str_res = _strcmp(*this, var1);
  return _str_res == 0;
}


void string_var::append(const string_var& str){
  _append_cstr(str._str_mem, str._mem_size);
}

void string_var::append(const std::string& str){
  _append_cstr(str.c_str(), str.size());
}

void string_var::append(const char* cstr){
  _append_cstr(cstr, std::strlen(cstr));
}

void string_var::append(const char* cstr, size_t len){
  _append_cstr(cstr, len);
}


const char* string_var::get_str_data() const{
  return _str_mem;
}

size_t string_var::get_str_len() const{
  return _mem_size;
}





// MARK: lua::number_var def
number_var::number_var(){
  _this_data = 0;
}

number_var::number_var(const double& from){
  _this_data = from;
}

number_var::number_var(lua_State* state, int stack_idx){
  from_state(state, stack_idx);
}


int number_var::_comp_num(const number_var& var1, const number_var& var2){
  int _i_var1 = (int)round(var1._this_data);
  int _i_var2 = (int)round(var2._this_data);

  double _delta_var1 = var1._this_data - (double)_i_var1;
  double _delta_var2 = var2._this_data - (double)_i_var2;
  
  // equal comparison
  if(
    abs(_delta_var1) <= NUM_VAR_EQUAL_ROUND_COMP && abs(_delta_var2) <= NUM_VAR_EQUAL_ROUND_COMP &&
    _i_var1 == _i_var2
    )
  {
    return 0;
  }
  
  return var1._this_data < var2._this_data? -1: 1;
}


int number_var::_compare_with(const variant* rhs) const{
  return _comp_num(*this, *(number_var*)rhs);
}


int number_var::get_type() const{
  return LUA_TNUMBER;
}


bool number_var::from_state(lua_State* state, int stack_idx){
  _this_data = 0;

  int _type = lua_type(state, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str("Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lua_typename(state, _type),
      lua_typename(state, get_type())
    ));

    return false;
  }

  _this_data = lua_tonumber(state, stack_idx);
  return true;
}

void number_var::push_to_stack(lua_State* state) const{
  lua_pushnumber(state, _this_data);
}

variant* number_var::create_copy() const{
  number_var* _result = new number_var();
  _result->_this_data = _this_data;

  return _result;
}


std::string number_var::to_string() const{
  int _str_len = snprintf(NULL, 0, "%f", _this_data);
  
  char* _c_str = (char*)malloc(_str_len+1);
  snprintf(_c_str, _str_len+1, "%f", _this_data);

  std::string _result = _c_str;
  free(_c_str);

  return _result;
}


double number_var::get_number() const{
  return _this_data;
}


number_var& number_var::operator=(const number_var& val){
  _this_data = val._this_data;
  return *this;
}

number_var& number_var::operator=(const double& val){
  _this_data = val;
  return *this;
}


number_var& number_var::operator+=(const number_var& var1){
  _this_data += var1._this_data;
  return *this;
}

number_var& number_var::operator-=(const number_var& var1){
  _this_data -= var1._this_data;
  return *this;
}

number_var& number_var::operator*=(const number_var& var1){
  _this_data *= var1._this_data;
  return *this;
}

number_var& number_var::operator/=(const number_var& var1){
  _this_data /= var1._this_data;
  return *this;
}


number_var number_var::operator+(const number_var& var1) const{
  return (number_var)(_this_data+var1._this_data);
}

number_var number_var::operator-(const number_var& var1) const{
  return (number_var)(_this_data-var1._this_data);
}

number_var number_var::operator*(const number_var& var1) const{
  return (number_var)(_this_data*var1._this_data);
}

number_var number_var::operator/(const number_var& var1) const{
  return (number_var)(_this_data/var1._this_data);
}


bool number_var::operator==(const number_var& var1) const{
  int _res_cmp = _comp_num(*this, var1);
  return _res_cmp == 0;
}

bool number_var::operator!=(const number_var& var1) const{
  int _res_cmp = _comp_num(*this, var1);
  return _res_cmp != 0;
}

bool number_var::operator<(const number_var& var1) const{
  int _res_cmp = _comp_num(*this, var1);
  return _res_cmp < 0;
}

bool number_var::operator>(const number_var& var1) const{
  int _res_cmp = _comp_num(*this, var1);
  return _res_cmp > 0;
}

bool number_var::operator<=(const number_var& var1) const{
  int _res_cmp = _comp_num(*this, var1);
  return _res_cmp <= 0;
}

bool number_var::operator>=(const number_var& var1) const{
  int _res_cmp = _comp_num(*this, var1);
  return _res_cmp >= 0;
}





// MARK: lua::bool_var def
bool_var::bool_var(){
  _this_data = true;
}

bool_var::bool_var(const bool& from){
  _this_data = from;
}

bool_var::bool_var(lua_State* state, int stack_idx){
  from_state(state, stack_idx);
}


int bool_var::_comp_bool(const bool_var& var1, const bool_var& var2){
  if(var1._this_data == var2._this_data)
    return 0;

  return var1._this_data < var2._this_data? -1: 1;
}


int bool_var::_compare_with(const variant* rhs) const{
  return _comp_bool(*this, *(bool_var*)rhs);
}


int bool_var::get_type() const{
  return LUA_TBOOLEAN;
}


bool bool_var::from_state(lua_State* state, int stack_idx){
  _this_data = true;

  int _type = lua_type(state, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str("Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lua_typename(state, _type),
      lua_typename(state, get_type())
    ));

    return false;
  }

  _this_data = lua_toboolean(state, stack_idx);
  return true;
}

void bool_var::push_to_stack(lua_State* state) const{
  lua_pushboolean(state, _this_data);
}

variant* bool_var::create_copy() const{
  bool_var* _result = new bool_var();
  _result->_this_data = _this_data;

  return _result;
}


std::string bool_var::to_string() const{
  return _this_data? "true": "false";
}


bool_var& bool_var::operator=(const bool_var& val){
  _this_data = val._this_data;
  return *this;
}

bool_var& bool_var::operator=(const bool& val){
  _this_data = val;
  return *this;
}


bool bool_var::operator==(const bool_var& var1) const{
  int _res_cmp = _comp_bool(*this, var1);
  return _res_cmp == 0;
}

bool bool_var::operator!=(const bool_var& var1) const{
  int _res_cmp = _comp_bool(*this, var1);
  return _res_cmp != 0;
}

bool bool_var::operator<(const bool_var& var1) const{
  int _res_cmp = _comp_bool(*this, var1);
  return _res_cmp < 0;
}

bool bool_var::operator>(const bool_var& var1) const{
  int _res_cmp = _comp_bool(*this, var1);
  return _res_cmp > 0;
}

bool bool_var::operator<=(const bool_var& var1) const{
  int _res_cmp = _comp_bool(*this, var1);
  return _res_cmp <= 0;  
}

bool bool_var::operator>=(const bool_var& var1) const{
  int _res_cmp = _comp_bool(*this, var1);
  return _res_cmp >= 0;
}


bool bool_var::operator!() const{
  return !_this_data;
}




struct _self_reference_data{
  std::set<const void*> referenced_data;
};

std::map<std::thread::id, _self_reference_data*>* _sr_data_map = NULL;

// MARK: lua::table_var
table_var::table_var(){
  _init_class();
}

table_var::table_var(const table_var& var){
  _init_class();
  _copy_from_var(var);
}

table_var::table_var(lua_State* state, int stack_idx){
  _init_class();
  from_state(state, stack_idx);
}


table_var::~table_var(){
  _clear_table_data();
}


void table_var::_init_class(){
  if(!_sr_data_map)
    _sr_data_map = new std::map<std::thread::id, _self_reference_data*>();
}


void table_var::_copy_from_var(const table_var& var){
  _clear_table_data();
  for(auto _iter: var._table_data)
    set_value(_iter.first, _iter.second);
}


void table_var::_clear_table_data(){
  for(auto _iter: _table_data)
    delete _iter.second;

  _table_data.clear();
}


void table_var::_fs_iter_cb(lua_State* state, int key_stack, int val_stack, int iter_idx, void* cb_data){
  int _i_debug = 0;

  table_var* _this = (table_var*)cb_data;

  int _type = lua_type(state, key_stack);
  variant* _key_value;
  switch(_type){
    break;
      case LUA_TSTRING:
      case LUA_TNUMBER:
      case LUA_TNIL:
      case LUA_TLIGHTUSERDATA:
      case LUA_TUSERDATA:
      case LUA_TBOOLEAN:{
        _key_value = to_variant(state, key_stack);
      }

    break; default:{
      _key_value = to_variant_ref(state, key_stack);
    }
  }

  variant* _value = to_variant(state, val_stack);
  if(_value->get_type() == LUA_TNIL){
    std::string _key_str = _key_value->to_string();
    _logger->print_warning(format_str("Value of (type %d %s) is Nil. Is this intended?\n", _key_value->get_type(), _key_str.c_str()));
  }

  switch(_key_value->get_type()){
    break;
      case lua::string_var::get_static_lua_type():
      case lua::nil_var::get_static_lua_type():
      case lua::bool_var::get_static_lua_type():
      case lua::lightuser_var::get_static_lua_type():
      case lua::number_var::get_static_lua_type():{
        comparison_variant _cmp_var = _key_value;
        _this->set_value(_cmp_var, _value);
      }

    break; default:{
      _logger->print_error(format_str("Using type (%s) as key is not supported.\n",
        lua_typename(state, _key_value->get_type())
      ));
    }
  }

  delete _key_value;
  delete _value;

  return;
}


variant* table_var::_get_value(const comparison_variant& comp_var) const{
  auto _iter = _table_data.find(comp_var);
  if(_iter == _table_data.end())
    return NULL;

  return _iter->second;
}


int table_var::get_type() const{
  return LUA_TTABLE;
}


bool table_var::from_state(lua_State* state, int stack_idx){
  _clear_table_data();

  int _type = lua_type(state, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str("Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lua_typename(state, _type),
      lua_typename(state, get_type())
    ));

    return false;
  }

  bool _has_sr_ownership = false;
  _self_reference_data* _sr_data;
  auto _iter = _sr_data_map->find(std::this_thread::get_id());
  if(_iter == _sr_data_map->end()){
    _has_sr_ownership = true;

    _sr_data = new _self_reference_data();
    _sr_data_map->operator[](std::this_thread::get_id()) = _sr_data;
  }
  else
    _sr_data = _iter->second;

  const void *_this_luapointer = lua_topointer(state, stack_idx);
  if(_sr_data->referenced_data.find(_this_luapointer) != _sr_data->referenced_data.end())
    goto skip_parsing_label;

  _sr_data->referenced_data.insert(_this_luapointer);
  iterate_table(state, stack_idx, _fs_iter_cb, this);
  
  skip_parsing_label:{
    if(_has_sr_ownership){
      _sr_data_map->erase(std::this_thread::get_id());
      delete _sr_data;
    }
  }

  return true;
}

void table_var::push_to_stack(lua_State* state) const{
  lua_newtable(state);

  for(auto _iter: _table_data)
    set_table_value(state, LUA_CONV_T2B(state, -1), _iter.first.get_comparison_data(), _iter.second);
}

variant* table_var::create_copy() const{
  return new table_var(*this);
}


std::string table_var::to_string() const{
  return "table";
}


variant* table_var::get_value(const comparison_variant& comp_var){
  return _get_value(comp_var);
}

const variant* table_var::get_value(const comparison_variant& comp_var) const{
  return _get_value(comp_var);
}


void table_var::set_value(const comparison_variant& comp_var, variant* value){
  remove_value(comp_var);
  _table_data[comp_var] = value->create_copy();
}

bool table_var::remove_value(const comparison_variant& comp_var){
  auto _check_iter = _table_data.find(comp_var);
  if(_check_iter == _table_data.end())
    return false;

  delete _check_iter->second;
  _table_data.erase(_check_iter);

  return true;
}





// MARK: lua::lightuser_var
lightuser_var::lightuser_var(){
  _pointer_data = NULL;
}

lightuser_var::lightuser_var(void* pointer){
  _pointer_data = pointer;
}

lightuser_var::lightuser_var(lua_State* state, int stack_idx){
  from_state(state, stack_idx);
}


int lightuser_var::_comp_lud(const lightuser_var& var1, const lightuser_var& var2){
  long long _p_rep1 = (long long)var1._pointer_data;
  long long _p_rep2 = (long long)var2._pointer_data;

  if(_p_rep1 == _p_rep2)
    return 0;

  return _p_rep1 < _p_rep2? -1: 1;
}


int lightuser_var::_compare_with(const variant* rhs) const{
  return _comp_lud(*this, *(lightuser_var*)rhs);
}


int lightuser_var::get_type() const{
  return LUA_TLIGHTUSERDATA;
}


bool lightuser_var::from_state(lua_State* state, int stack_idx){
  int _type = lua_type(state, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str("Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lua_typename(state, _type),
      lua_typename(state, get_type())
    ));

    return false;
  }

  _pointer_data = lua_touserdata(state, stack_idx);
  return true;
}

void lightuser_var::push_to_stack(lua_State* state) const{
  lua_pushlightuserdata(state, _pointer_data);
}

variant* lightuser_var::create_copy() const{
  return new lightuser_var(_pointer_data);
}


std::string lightuser_var::to_string() const{
  return format_str("lud 0x%X", _pointer_data);
}


const void* lightuser_var::get_data() const{
  return _pointer_data;
}

void* lightuser_var::get_data(){
  return _pointer_data;
}


bool lightuser_var::operator==(const lightuser_var& var1) const{
  int _res_cmp = _comp_lud(*this, var1);
  return _res_cmp == 0;
}

bool lightuser_var::operator!=(const lightuser_var& var1) const{
  int _res_cmp = _comp_lud(*this, var1);
  return _res_cmp != 0;
}

bool lightuser_var::operator<(const lightuser_var& var1) const{
  int _res_cmp = _comp_lud(*this, var1);
  return _res_cmp < 0;
}

bool lightuser_var::operator>(const lightuser_var& var1) const{
  int _res_cmp = _comp_lud(*this, var1);
  return _res_cmp > 0;
}

bool lightuser_var::operator<=(const lightuser_var& var1) const{
  int _res_cmp = _comp_lud(*this, var1);
  return _res_cmp <= 0;
}

bool lightuser_var::operator>=(const lightuser_var& var1) const{
  int _res_cmp = _comp_lud(*this, var1);
  return _res_cmp >= 0;
}





// MARK: lua::error_var
error_var::error_var(){
  _err_data = new variant();
}

error_var::error_var(const error_var& var){
  _err_data = var._err_data? var._err_data->create_copy(): NULL;
  _error_code = var._error_code;
}

error_var::error_var(lua_State* state, int stack_idx){
  from_state(state, stack_idx);
}


int error_var::get_type() const{
  return LUA_TERROR;
}


bool error_var::from_state(lua_State* state, int stack_idx){
  if(_err_data)
    delete _err_data;

  _err_data = to_variant(state, stack_idx);
  return true;
}

void error_var::push_to_stack(lua_State* state) const{
  if(!_err_data)
    return;

  _err_data->push_to_stack(state);
}

variant* error_var::create_copy() const{
  return new error_var(*this);
}


std::string error_var::to_string() const{
  if(!_err_data)
    return "nil";

  std::string _err_str = _err_data->to_string();
  return format_str("(%d) %s", _error_code, _err_str.c_str());
}


const variant* error_var::get_error_data() const{
  return _err_data;
}

int error_var::get_error_code() const{
  return _error_code;
}

void error_var::set_error_code(int code){
  _error_code = code;
}




// MARK: lua::comparison_variant def
comparison_variant::comparison_variant(const variant* from){
  _init_class(from);
}

comparison_variant::comparison_variant(const comparison_variant& from){
  _init_class(from._this_var);
}

comparison_variant::comparison_variant(const string_var& from){
  _init_class((const variant*)&from);
}

comparison_variant::comparison_variant(const number_var& from){
  _init_class((const variant*)&from);
}

comparison_variant::comparison_variant(const bool_var& from){
  _init_class((const variant*)&from);
}

comparison_variant::comparison_variant(const lightuser_var& from){
  _init_class((const variant*)&from);
}

comparison_variant::~comparison_variant(){
  delete _this_var;
}


void comparison_variant::_init_class(const variant* from){
  _this_var = from->create_copy();
}


bool comparison_variant::operator<(const comparison_variant& rhs) const{
  int _cmp_res = _this_var->compare_with(rhs._this_var);
  return _cmp_res < 0;
}

bool comparison_variant::operator>(const comparison_variant& rhs) const{
  int _cmp_res = _this_var->compare_with(rhs._this_var);
  return _cmp_res > 0;
}

bool comparison_variant::operator<=(const comparison_variant& rhs) const{
  int _cmp_res = _this_var->compare_with(rhs._this_var);
  return _cmp_res <= 0;
}

bool comparison_variant::operator>=(const comparison_variant& rhs) const{
  int _cmp_res = _this_var->compare_with(rhs._this_var);
  return _cmp_res >= 0;
}

bool comparison_variant::operator==(const comparison_variant& rhs) const{
  int _cmp_res = _this_var->compare_with(rhs._this_var);
  return _cmp_res == 0;
}

bool comparison_variant::operator!=(const comparison_variant& rhs) const{
  int _cmp_res = _this_var->compare_with(rhs._this_var);
  return _cmp_res != 0;
}


const variant* comparison_variant::get_comparison_data() const{
  return _this_var;
}




// MARK: lua::to_variant
variant* lua::to_variant(lua_State* state, int stack_idx){
  variant* _result;

  int _type = lua_type(state, stack_idx);
  switch(_type){
    break; case number_var::get_static_lua_type():{
      _result = new number_var(state, stack_idx);
    }

    break; case string_var::get_static_lua_type():{
      _result = new string_var(state, stack_idx);
    }

    break; case bool_var::get_static_lua_type():{
      _result = new bool_var(state, stack_idx);
    }

    break; case table_var::get_static_lua_type():{
      _result = new table_var(state, stack_idx);
    }

    break; case lightuser_var::get_static_lua_type():{
      _result = new lightuser_var(state, stack_idx);
    }

    break; default:{
      _result = new variant();
    }
  }

  return _result;
}


// MARK:: lua::to_variant_fglobal
variant* lua::to_variant_fglobal(lua_State* state, const char* global_name){
  lua_getglobal(state, global_name);
  variant* _result = lua::to_variant(state, -1);
  lua_pop(state, 1);

  return _result;
}

// MARK:: lua::to_variant_ref
variant* lua::to_variant_ref(lua_State* state, int stack_idx){
  const void* _pointer_ref = lua_topointer(state, stack_idx);
  string_var* _var_res = new string_var(format_str("p_ref 0x%X", _pointer_ref));

  return _var_res;
}


// MARK: lua::from_pointer_to_str
string_var lua::from_pointer_to_str(void* pointer){
  long long _pointer_num = (long long)pointer;
  return string_var(reinterpret_cast<const char*>(_pointer_num), sizeof(long long));
}


// MARK: lua::from_str_to_pointer
void* lua::from_str_to_pointer(const string_var& str){
  long long _pointer_num = *(reinterpret_cast<const long long*>(str.get_str_data()));
  return (void*)_pointer_num;
}