#include "lua_str.h"
#include "lua_vararr.h"
#include "lua_variant.h"
#include "luacpp_error_handling.h"
#include "luaobject_helper.h"
#include "luastack_iter.h"
#include "luatable_util.h"
#include "luautility.h"
#include "stdlogger.h"
#include "strutil.h"

#include "cmath"
#include "cstring"
#include "thread"
#include "set"

#define BASE_VARIANT_TYPE_NAME "nil"

#define NUM_VAR_EQUAL_ROUND_COMP 0.01

#define TABLE_REFERENCE_GLOBAL_DATA "__clua_table_ref_data"
#define TABLE_METADATA_ACTUAL_VALUE "value"
#define TABLE_METADATA_REFERENCE_COUNT "ref_count"

#define FUNCTION_REFERENCE_GLOBAL_DATA "__clua_func_ref_data"
#define FUNCTION_METADATA_ACTUAL_VALUE "value"
#define FUNCTION_METADATA_REFERENCE_COUNT "ref_count"


using namespace lua;
using namespace lua::api;
using namespace lua::utility;


stdlogger _default_logger = stdlogger();
const I_logger* _logger = &_default_logger;



// MARK: set_default_logger

void lua::set_default_logger(I_logger* logger){
  _logger = logger;
}




// MARK: variant def

variant::variant(){}


int variant::get_type() const{
  return LUA_TNIL;
}


void variant::to_string(I_string_store* pstring) const{
  pstring->append(BASE_VARIANT_TYPE_NAME);
}

std::string variant::to_string() const{
  return BASE_VARIANT_TYPE_NAME;
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




// MARK: string_var def

string_var::string_var(){
  _init_class();
}

string_var::string_var(const string_var& var1){
  _init_class();
  _set_to_cstr(var1._str_mem, var1._mem_size);
}

string_var::string_var(const char* cstr){
  _init_class();

  std::size_t _str_len = std::strlen(cstr);
  _set_to_cstr(cstr, _str_len);
}

string_var::string_var(const char* cstr, std::size_t len){
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


void string_var::_resize_mem(std::size_t newlen){
  _str_mem = (char*)realloc(_str_mem, newlen+1);
  _mem_size = newlen;
}


void string_var::_set_to_cstr(const char* cstr, std::size_t strlen){
  _resize_mem(strlen);

  memcpy(_str_mem, cstr, strlen);
  _put_delimiter_at_end();
}

void string_var::_append_cstr(const char* cstr, std::size_t strlen){
  std::size_t _original_len = _mem_size;
  _resize_mem(_mem_size + strlen);

  char* _offset_str_mem = (char*)((long long)(_str_mem + _original_len));
  memcpy(_offset_str_mem, cstr, strlen);
  _put_delimiter_at_end();
}


int string_var::_strcmp(const string_var& var1, const string_var& var2){
  std::size_t _check_len = std::min(var1._mem_size, var2._mem_size);
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


string_var* string_var::create_copy_static(const I_string_var* data){
  return new string_var(data->get_string(), data->get_length());
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
      ).c_str());

      return false;
    }
  }

  std::size_t _str_len;
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


void string_var::to_string(I_string_store* pstring) const{
  return pstring->append(_str_mem, _mem_size);
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


const char& string_var::operator[](std::size_t i) const{
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

void string_var::append(const char* cstr, std::size_t len){
  _append_cstr(cstr, len);
}


const char* string_var::get_string() const{
  return _str_mem;
}

std::size_t string_var::get_length() const{
  return _mem_size;
}





// MARK: number_var def

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


number_var* number_var::create_copy_static(const I_number_var* data){
  return new number_var(data->get_number());
}


bool number_var::from_state(lua_State* state, int stack_idx){
  _this_data = 0;

  int _type = lua_type(state, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str("Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lua_typename(state, _type),
      lua_typename(state, get_type())
    ).c_str());

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


void number_var::to_string(I_string_store* data) const{
  std::string _str_data = to_string();
  data->append(_str_data.c_str());
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

void number_var::set_number(double num){
  _this_data = num;
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





// MARK: bool_var def

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


bool_var* bool_var::create_copy_static(const I_bool_var* data){
  return new bool_var(data->get_boolean());
}


bool bool_var::from_state(lua_State* state, int stack_idx){
  _this_data = true;

  int _type = lua_type(state, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str("Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lua_typename(state, _type),
      lua_typename(state, get_type())
    ).c_str());

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


#define BOOL_VAR_TO_STRING(b) b? "true": "false" 

void bool_var::to_string(I_string_store* data) const{
  return data->append(BOOL_VAR_TO_STRING(_this_data));
}

std::string bool_var::to_string() const{
  return BOOL_VAR_TO_STRING(_this_data);
}


bool bool_var::get_boolean() const{
  return _this_data;
}

void bool_var::set_boolean(bool b){
  _this_data = b;
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



// MARK: table_var def
// NOTE: API interfaces only used when it is using a reference.

struct _self_reference_data{
  std::set<const void*> referenced_data;
};

std::map<std::thread::id, _self_reference_data*>* _sr_data_map = NULL;


// Create copy of a table
table_var::table_var(){
  _init_class();

  _table_data = new std::map<comparison_variant, variant*>();
  _is_ref = false;
}

// Copy attributes from a table variant
table_var::table_var(const table_var& var){
  _init_class();
  _copy_from_var(var);
}

// Create a reference
table_var::table_var(lua_State* state, int stack_idx){
  _init_class();
  from_state(state, stack_idx);
}

table_var::table_var(void* istate, int stack_idx, const compilation_context* context){
  _init_class();
  from_state(istate, stack_idx, context);
}


table_var::~table_var(){
  _clear_table_data();

  _clear_keys_reg();
  free(_keys_buffer);
}


void table_var::_init_class(){
  if(!_sr_data_map)
    _sr_data_map = new std::map<std::thread::id, _self_reference_data*>();

  _init_keys_reg();
}


void table_var::_init_table_ref(){
  
}

void table_var::_init_table_data(){
  if(_table_data)
    return;

  _table_data = new std::map<comparison_variant, variant*>();
}


void table_var::_copy_from_var(const table_var& var){
  _is_ref = var._is_ref;
  if(_is_ref){
    if(!_table_pointer)
      return;

    // assume that there are already reference metadata, and we only need to increment the reference
    _table_pointer = var._table_pointer;

    _lua_state = var._lua_state;

    _api_stack = var._api_stack;
    _api_value = var._api_value;
    
    _api_var = var._api_var;
    _api_table = var._api_table;

    _increment_reference();
  }
  else{
    _table_data = new std::map<comparison_variant, variant*>();

    for(auto _iter: *var._table_data)
      set_value(_iter.first, _iter.second);
  }
}

void table_var::_clear_table_data(){
  if(_table_pointer){
    if(_get_reference_count() > 1)
      _decrement_reference();
    else
      _delete_reference();

    _table_pointer = NULL;
  }

  if(_table_data){
    for(auto _iter: *_table_data)
      delete _iter.second;

    _table_data->clear();
    delete _table_data;

    _table_data = NULL;
  }
}


void table_var::_create_reference(){
  if(!_table_pointer)
    return;

  _require_global_table();

  std::string _key_address = _get_reference_key(_table_pointer);
  _api_value->pushstring(_lua_state, _key_address.c_str());
  _api_value->newtable(_lua_state);
  
  // set reference count
  _api_value->pushstring(_lua_state, TABLE_METADATA_REFERENCE_COUNT);
  _api_value->pushinteger(_lua_state, 1);
  _api_value->settable(_lua_state, -3);

  // set table value
  _api_value->pushstring(_lua_state, TABLE_METADATA_ACTUAL_VALUE);
  _api_stack->pushvalue(_lua_state, -5); // copy table value
  _api_value->settable(_lua_state, -3);

  // set the metadata to the global list
  _api_value->settable(_lua_state, -3);

  // pop the global table
  _api_stack->pop(_lua_state, 1);
}

void table_var::_delete_reference(){
  if(!_table_pointer)
    return;

  _require_global_table();

  // remove reference by setting it to nil
  std::string _key_address = _get_reference_key(_table_pointer);
  _api_value->pushstring(_lua_state, _key_address.c_str());
  _api_value->pushnil(_lua_state);
  _api_value->settable(_lua_state, -3);

  // pop the global table
  _api_stack->pop(_lua_state, 1);
}

void table_var::_require_global_table() const{
  if(!_table_pointer)
    return;

  int _ret_type = _api_value->getglobal(_lua_state, TABLE_REFERENCE_GLOBAL_DATA);
  if(_ret_type != LUA_TNIL)
    return;
  
  // pop the nil value first
  _api_stack->pop(_lua_state, 1);
  
  _api_value->newtable(_lua_state);
  
  // create copy for caller
  _api_stack->pushvalue(_lua_state, -1);
  _api_value->setglobal(_lua_state, TABLE_REFERENCE_GLOBAL_DATA);
}


void table_var::_push_metadata_table() const{
  if(!_table_pointer)
    return;

  // get metadata list s+1
  _require_global_table();

  // get metadata s+1
  std::string _key_address = _get_reference_key(_table_pointer);
  _api_value->pushstring(_lua_state, _key_address.c_str());
  _api_value->gettable(_lua_state, -2);

  // set the top result as the result and trim the stack
  _api_stack->insert(_lua_state, -2);
  _api_stack->pop(_lua_state, 1);
}

void table_var::_push_actual_table() const{
  if(!_table_pointer)
    return;

  // get metadata s+1
  _push_metadata_table();

  // get actual table s+1
  _api_value->pushstring(_lua_state, TABLE_METADATA_ACTUAL_VALUE);
  _api_value->gettable(_lua_state, -2);

  // set the top result as the result and trim the stack
  _api_stack->insert(_lua_state, -2);
  _api_stack->pop(_lua_state, 1);
}


void table_var::_increment_reference(){
  if(!_table_pointer)
    return;

  // get metadata s+1
  _push_metadata_table();

  _api_value->pushstring(_lua_state, TABLE_METADATA_REFERENCE_COUNT);
  _api_stack->pushvalue(_lua_state, -1); // copy for later use
  _api_value->gettable(_lua_state, -3);

  // increment reference
  _api_value->pushinteger(_lua_state, 1);
  _api_value->arith(_lua_state, LUA_OPADD);

  _api_value->settable(_lua_state, -3);

  _api_stack->pop(_lua_state, 1);
}

void table_var::_decrement_reference(){
  if(!_table_pointer)
    return;

  // get metadata s+1
  _push_metadata_table();

  _api_value->pushstring(_lua_state, TABLE_METADATA_REFERENCE_COUNT);
  _api_stack->pushvalue(_lua_state, -1); // copy for later use
  _api_value->gettable(_lua_state, -3);

  // decrement reference
  _api_value->pushinteger(_lua_state, 1);
  _api_value->arith(_lua_state, LUA_OPSUB);

  _api_value->settable(_lua_state, -3);

  _api_stack->pop(_lua_state, 1);
}

int table_var::_get_reference_count(){
  if(!_table_pointer)
    return 0;

  // get metadata s+1
  _push_metadata_table();

  _api_value->pushstring(_lua_state, TABLE_METADATA_REFERENCE_COUNT);
  _api_value->gettable(_lua_state, -2);

  int _res = _api_value->tointeger(_lua_state, -1);
  _api_stack->pop(_lua_state, 2);

  return _res;
}


bool table_var::_has_reference_metadata(){
  if(!_table_pointer)
    return false;

  return pstack_call<bool>(_lua_state, 0, 0, _api_stack, _api_value, [](table_var* _this){
    _this->_require_global_table();
    if(_this->_api_value->type(_this->_lua_state, -1) != LUA_TTABLE)
      return false;

    std::string _key_address = table_var::_get_reference_key(_this->_table_pointer);
    _this->_api_value->pushstring(_this->_lua_state, _key_address.c_str());
    _this->_api_value->gettable(_this->_lua_state, -2);

    return _this->_api_value->type(_this->_lua_state, -1) == LUA_TTABLE;
  }, this);
}


std::string table_var::_get_reference_key(const void* pointer){
  return format_str("0x%X", (unsigned long long)pointer);
}


void table_var::_fs_iter_cb(lua_State* state, int key_stack, int val_stack, int iter_idx, void* cb_data){
  int _i_debug = 0;

  table_var* _this = (table_var*)cb_data;

  int _type = _this->_api_value->type(state, key_stack);
  I_variant* _key_value;
  switch(_type){
    break;
      case LUA_TSTRING:
      case LUA_TNUMBER:
      case LUA_TNIL:
      case LUA_TLIGHTUSERDATA:
      case LUA_TUSERDATA:
      case LUA_TBOOLEAN:{
        _key_value = _this->_api_var->to_variant(state, key_stack);
      }

    break; default:{
      _key_value = _this->_api_var->to_variant_ref(state, key_stack);
    }
  }

  I_variant* _value = _this->_api_var->to_variant(state, val_stack);
  if(_value->get_type() == LUA_TNIL){
    string_store _str;
    _key_value->to_string(&_str);
    _logger->print_warning(format_str("Value of (type %d %s) is Nil. Is this intended?\n", _key_value->get_type(), _str.data.c_str()).c_str());
  }

  switch(_key_value->get_type()){
    break;
      case lua::string_var::get_static_lua_type():
      case lua::nil_var::get_static_lua_type():
      case lua::bool_var::get_static_lua_type():
      case lua::lightuser_var::get_static_lua_type():
      case lua::number_var::get_static_lua_type():{
        _this->set_value(_key_value, _value);
      }

    break; default:{
      _logger->print_error(format_str("Using type (%s) as key is not supported.\n",
        _this->_api_value->ttypename(state, _key_value->get_type())
      ).c_str());
    }
  }

  _this->_api_var->delete_variant(_key_value);
  _this->_api_var->delete_variant(_value);

  return;
}

bool table_var::_from_state_copy_by_interface(void* istate, int stack_idx){
  _clear_table_data();
  _table_data = new std::map<comparison_variant, variant*>();

  int _type = _api_value->type(istate, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str("Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      _api_value->ttypename(istate, _type),
      _api_value->ttypename(istate, get_type())
    ).c_str());

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

  const void *_this_luapointer = _api_value->topointer(istate, stack_idx);
  if(_sr_data->referenced_data.find(_this_luapointer) != _sr_data->referenced_data.end())
    goto skip_parsing_label;

  _sr_data->referenced_data.insert(_this_luapointer);
  _api_table->iterate_table(istate, stack_idx, _fs_iter_cb, this);
  
  skip_parsing_label:{
    if(_has_sr_ownership){
      _sr_data_map->erase(std::this_thread::get_id());
      delete _sr_data;
    }
  }

  return true;
}

bool table_var::_from_state_ref_by_interface(void* istate, int stack_idx){
  _clear_table_data();

  int _type = _api_value->type(istate, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str("Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      _api_value->ttypename(istate, _type),
      _api_value->ttypename(istate, get_type())
    ).c_str());

    return false;
  }

  _lua_state = istate;
  _table_pointer = _api_value->topointer(istate, stack_idx);

  if(_has_reference_metadata())
    _increment_reference();
  else{
    _api_stack->pushvalue(istate, stack_idx);
    _create_reference();
    _api_stack->pop(istate, 1);
  }

  return true;
}


variant* table_var::_get_value_by_interface(const I_variant* key) const{
  if(!_table_pointer)
    return NULL;

  // get actual table s+1
  _push_actual_table();

  // get value s+1
  _api_var->push_variant(_lua_state, key);
  _api_value->gettable(_lua_state, -2);

  // create compilation specific copy
  // TODO need optimization
  I_variant* _res_tmp = _api_var->to_variant(_lua_state, -1);
  variant* _res = cpplua_create_var_copy(_res_tmp);
  _api_var->delete_variant(_res_tmp);

  _api_stack->pop(_lua_state, 2);
  return _res;
}

void table_var::_set_value_by_interface(const I_variant* key, const I_variant* value){
  if(!_table_pointer)
    return;

  // get actual table s+1
  _push_actual_table();

  // set value
  _api_var->push_variant(_lua_state, key);
  _api_var->push_variant(_lua_state, value);
  _api_value->settable(_lua_state, -3);

  _api_stack->pop(_lua_state, 1);

  _update_keys_reg();
}

bool table_var::_remove_value_by_interface(const I_variant* key){
  if(!_table_pointer)
    return false;

  // get actual table s+1
  _push_actual_table();

  // set value to nil
  _api_var->push_variant(_lua_state, key);
  _api_value->pushnil(_lua_state);
  _api_value->settable(_lua_state, -3);

  _api_stack->pop(_lua_state, 1);

  _update_keys_reg();
  return true;
}


variant* table_var::_get_value(const comparison_variant& comp_var) const{
  auto _iter = _table_data->find(comp_var);
  if(_iter == _table_data->end())
    return NULL;

  return cpplua_create_var_copy(_iter->second);
}

variant* table_var::_get_value(const I_variant* key) const{
  variant* _key_data = cpplua_create_var_copy(key);
  comparison_variant _key_comp(_key_data); delete _key_data;
  
  return _get_value(_key_comp);
}


void table_var::_set_value(const comparison_variant& comp_key, const variant* value){
  _remove_value(comp_key);
  _table_data->operator[](comp_key) = value->create_copy();

  _update_keys_reg();
}

void table_var::_set_value(const I_variant* key, const I_variant* value){
  variant* _key_data = cpplua_create_var_copy(key);
  comparison_variant _key_comp(_key_data);

  variant* _value_data = cpplua_create_var_copy(value);
  _set_value(_key_comp, _value_data);

  delete _key_data;
  delete _value_data;
}


bool table_var::_remove_value(const comparison_variant& comp_key){
  auto _check_iter = _table_data->find(comp_key);
  if(_check_iter == _table_data->end())
    return false;

  delete _check_iter->second;
  _table_data->erase(comp_key);

  _update_keys_reg();
  return true;
}

bool table_var::_remove_value(const I_variant* key){
  variant* _key_data = cpplua_create_var_copy(key);
  comparison_variant _key_comp(_key_data); delete _key_data;

  return _remove_value(_key_comp);
}


void table_var::_init_keys_reg(){
  _keys_buffer = (const I_variant**)malloc(sizeof(I_variant*));
  _keys_buffer[0] = NULL;
}

void table_var::_update_keys_reg(){
  _clear_keys_reg();

  if(_is_ref){
    if(!_table_pointer)
      return;

    // get actual table s+1
    _push_actual_table();

    int _key_len = _api_table->table_len(_lua_state, -1);
    _keys_buffer = (const I_variant**)realloc(_keys_buffer, (_key_len+1) * sizeof(I_variant*));

    struct iter_data{
      table_var* _this;
      int current_idx;
    }; iter_data _iterd;

    _iterd._this = this;
    _iterd.current_idx = 0;

    _api_table->iterate_table(_lua_state, -1, [](lua_State* state, int key_stack, int val_stack, int iter_idx, void* cb_data){
      iter_data* _iterd = (iter_data*)cb_data;
      
      I_variant* _ivar = _iterd->_this->_api_var->to_variant(state, key_stack);
      _iterd->_this->_keys_buffer[_iterd->current_idx] = cpplua_create_var_copy(_ivar);
      _iterd->_this->_api_var->delete_variant(_ivar);

      _iterd->current_idx++;
    }, &_iterd);
    
    _api_stack->pop(_lua_state, 1);
    _keys_buffer[_key_len] = NULL;
  }
  else{
    _keys_buffer = (const I_variant**)realloc(_keys_buffer, (_table_data->size()+1) * sizeof(I_variant*));

    int _idx = 0;
    for(auto _pair: *_table_data){
      I_variant* _key_data = cpplua_create_var_copy(_pair.first.get_comparison_data());
      _keys_buffer[_idx] = _key_data;

      _idx++;
    }

    _keys_buffer[_table_data->size()] = NULL;
  }
}

void table_var::_clear_keys_reg(){
  int _idx = 0;
  while(_keys_buffer[_idx]){
    delete _keys_buffer[_idx];
    _idx++;
  }

  _keys_buffer = (const I_variant**)realloc(_keys_buffer, sizeof(I_variant*));
  _keys_buffer[0] = NULL;
}


int table_var::get_type() const{
  return LUA_TTABLE;
}


table_var* table_var::create_copy_static(const I_table_var* data){
  table_var* _result = new table_var();
  _result->_clear_table_data();

  _result->_is_ref = data->is_reference();
  if(_result->_is_ref){
    // assume that there are already reference metadata, and we only need to increment the reference
    _result->_lua_state = data->get_lua_interface();
    _result->_table_pointer = data->get_table_pointer();
    _result->_api_stack = data->get_lua_stack_api_interface();
    _result->_api_value = data->get_lua_value_api_interface();
    _result->_api_var = data->get_lua_variant_api_interface();
    _result->_api_table = data->get_lua_table_api_interface();

    _result->_init_table_ref();

    _result->_increment_reference();
    _result->_update_keys_reg();
  }
  else{
    _result->_init_table_data();

    const I_variant** _key_arr = data->get_keys();

    int _idx = 0;
    while(_key_arr[_idx]){
      const I_variant* _key_data = _key_arr[_idx];
      const I_variant* _value_data = data->get_value(_key_data);
      _result->set_value(_key_data, _value_data);

      _idx++;
    }
  }

  return _result;
}


bool table_var::from_state(lua_State* state, int stack_idx){
  _api_stack = cpplua_get_api_stack_definition();
  _api_value = cpplua_get_api_value_definition();

  _api_table = cpplua_get_api_table_util_definition();
  _api_var = cpplua_get_api_variant_util_definition();

  return _from_state_ref_by_interface(state, stack_idx);
}

bool table_var::from_state(void* istate, int stack_idx, const compilation_context* context){
  _api_stack = context->api_stack;
  _api_value = context->api_value;

  _api_table = context->api_tableutil;
  _api_var = context->api_varutil;

  return _from_state_ref_by_interface(istate, stack_idx);
}

bool table_var::from_state_copy(lua_State* state, int stack_idx){
  _api_value = cpplua_get_api_value_definition();
  _api_table = cpplua_get_api_table_util_definition();
  _api_var = cpplua_get_api_variant_util_definition();

  return _from_state_copy_by_interface(state, stack_idx);
}


void table_var::push_to_stack(lua_State* state) const{
  if(_is_ref){
    if(!_table_pointer){
      _api_value->pushnil(state);
      return;
    }

    if(state == _lua_state)
      _push_actual_table();
    else{ // prohibit if the state address (in memory) is not the same
      table_var* _tmp_var = dynamic_cast<table_var*>(create_table_copy());
      _tmp_var->push_to_stack(state);
      cpplua_delete_variant(_tmp_var);
    }
  }
  else{
    lua_newtable(state);

    for(auto _iter: *_table_data)
      set_table_value(state, LUA_CONV_T2B(state, -1), _iter.first.get_comparison_data(), _iter.second);
  }
}

variant* table_var::create_copy() const{
  return new table_var(*this);
}


#define TABLE_VAR_TO_STRING() "table"

void table_var::to_string(I_string_store* pstring) const{
  pstring->append(TABLE_VAR_TO_STRING());
}

std::string table_var::to_string() const{
  return TABLE_VAR_TO_STRING();
}


I_table_var* table_var::create_table_copy() const{
  table_var* _res = new table_var();

  if(_is_ref){ // create copy from reference
    if(!_table_pointer)
      goto skip_label;
  
    // push the table to stack
    _push_actual_table();

    _res->_api_table = _api_table;
    _res->_api_value = _api_value;
    _res->_api_var = _api_var;

    _res->_from_state_copy_by_interface(_lua_state, -1);
  }
  else // just copy the table_var object
    _res->_copy_from_var(*this);

  skip_label:{}
  return _res;
}


const I_variant** table_var::get_keys() const{
  return _keys_buffer;
}

void table_var::update_keys(){
  _update_keys_reg();
}


variant* table_var::get_value(const comparison_variant& comp_var){
  return _is_ref? _get_value_by_interface(comp_var.get_comparison_data()): _get_value(comp_var);
}

I_variant* table_var::get_value(const I_variant* key){
  return _is_ref? _get_value_by_interface(key): _get_value(key);
}


const variant* table_var::get_value(const comparison_variant& comp_var) const{
  return _is_ref? _get_value_by_interface(comp_var.get_comparison_data()): _get_value(comp_var);
}

const I_variant* table_var::get_value(const I_variant* key) const{
  return _is_ref? _get_value_by_interface(key): _get_value(key);
}


void table_var::set_value(const comparison_variant& comp_var, variant* value){
  if(_is_ref)
    _set_value_by_interface(comp_var.get_comparison_data(), value);
  else
    _set_value(comp_var, value);
}

void table_var::set_value(const I_variant* key, const I_variant* data){
  if(_is_ref)
    _set_value_by_interface(key, data);
  else
    _set_value(key, data);
}


bool table_var::remove_value(const comparison_variant& comp_var){
  return _is_ref? _remove_value_by_interface(comp_var.get_comparison_data()): _remove_value(comp_var);
}

bool table_var::remove_value(const I_variant* key){
  return _is_ref? _remove_value_by_interface(key): _remove_value(key);
}


bool table_var::is_reference() const{
  return _is_ref;
}

const void* table_var::get_table_pointer() const{
  return _table_pointer;
}

void* table_var::get_lua_interface() const{
  return _lua_state;
}

lua::api::I_value* table_var::get_lua_value_api_interface() const{
  return _api_value;
}

lua::api::I_stack* table_var::get_lua_stack_api_interface() const{
  return _api_stack;
}

lua::api::I_table_util* table_var::get_lua_table_api_interface() const{
  return _api_table;
}

lua::api::I_variant_util* table_var::get_lua_variant_api_interface() const{
  return _api_var;
}





// MARK: lightuser_var def

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


lightuser_var* lightuser_var::create_copy_static(const I_lightuser_var* data){
  return new lightuser_var(data->get_data());
}


bool lightuser_var::from_state(lua_State* state, int stack_idx){
  int _type = lua_type(state, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str("Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lua_typename(state, _type),
      lua_typename(state, get_type())
    ).c_str());

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


#define LIGHTUSER_VAR_TO_STRING(ptr) format_str("lud 0x%X", ptr)

void lightuser_var::to_string(I_string_store* pstring) const{
  pstring->append(LIGHTUSER_VAR_TO_STRING(_pointer_data).c_str());
}

std::string lightuser_var::to_string() const{
  return LIGHTUSER_VAR_TO_STRING(_pointer_data);
}


void* lightuser_var::get_data() const{
  return _pointer_data;
}


void lightuser_var::set_data(void* data){
  _pointer_data = data;
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




// MARK: function_var def

function_var::function_var(luaapi_cfunction fn, const I_vararr* args){
  _init_class();

  _this_fn = fn;

  if(args){
    for(int i = 0; i < args->get_var_count(); i++)
      _fn_args->append_var(args->get_var(i));
  }
}

function_var::function_var(lua_State* state, int stack_idx){
  _init_class();
  from_state(state, stack_idx);
}

function_var::~function_var(){
  clear_function();
}


void function_var::_init_class(){
  _init_cfunction();
}

void function_var::_init_cfunction(){
  if(_fn_args)
    return;

  _fn_args = new vararr();
}


void function_var::_create_reference(){
  if(!_luafunc_pointer)
    return;

  _require_global_table();

  std::string _pkey = _get_reference_key(_luafunc_pointer);
  _api_value->pushstring(_lua_state, _pkey.c_str());
  _api_value->newtable(_lua_state);

  // set reference count
  _api_value->pushstring(_lua_state, FUNCTION_METADATA_REFERENCE_COUNT);
  _api_value->pushinteger(_lua_state, 1);
  _api_value->settable(_lua_state, -3);

  // set function value
  _api_value->pushstring(_lua_state, FUNCTION_METADATA_ACTUAL_VALUE);
  _api_stack->pushvalue(_lua_state, -5); // copy function value
  _api_value->settable(_lua_state, -3);

  // set the metadata to the global list
  _api_value->settable(_lua_state, -3);

  // pop the global table
  _api_stack->pop(_lua_state, 1);
}

void function_var::_delete_reference(){
  if(!_luafunc_pointer)
    return;

  _require_global_table();

  // remove reference by setting it to nil
  std::string _pkey = _get_reference_key(_luafunc_pointer);
  _api_value->pushstring(_lua_state, _pkey.c_str());
  _api_value->pushnil(_lua_state);
  _api_value->settable(_lua_state, -3);

  // pop the global table
  _api_stack->pop(_lua_state, 1);
}

void function_var::_require_global_table() const{
  if(!_luafunc_pointer)
    return;

  int _ret_type = _api_value->getglobal(_lua_state, FUNCTION_REFERENCE_GLOBAL_DATA);
  if(_ret_type != LUA_TNIL)
    return;

  // pop the nil value first
  _api_stack->pop(_lua_state, 1);

  _api_value->newtable(_lua_state);

  // create copy for caller
  _api_stack->pushvalue(_lua_state, -1);
  _api_value->setglobal(_lua_state, FUNCTION_REFERENCE_GLOBAL_DATA);
}


void function_var::_push_metadata_table() const{
  if(!_luafunc_pointer)
    return;

  // get metadata list s+1
  _require_global_table();

  // get metadata s+1
  std::string _pkey = _get_reference_key(_luafunc_pointer);
  _api_value->pushstring(_lua_state, _pkey.c_str());
  _api_value->gettable(_lua_state, -2);

  // set the top result as the result and trim the stack
  _api_stack->insert(_lua_state, -2);
  _api_stack->pop(_lua_state, 1);
}

void function_var::_push_actual_function() const{
  if(!_luafunc_pointer)
    return;

  // get metadata s+1
  _push_metadata_table();

  // get actual function s+1
  _api_value->pushstring(_lua_state, FUNCTION_METADATA_ACTUAL_VALUE);
  _api_value->gettable(_lua_state, -2);

  // set the top result as the result and trim the stack
  _api_stack->insert(_lua_state, -2);
  _api_stack->pop(_lua_state, 1);
}


void function_var::_increment_reference(){
  if(!_luafunc_pointer)
    return;

  // get metadata s+1
  _push_metadata_table();

  _api_value->pushstring(_lua_state, FUNCTION_METADATA_REFERENCE_COUNT);
  _api_stack->pushvalue(_lua_state, -1); // copy for later use
  _api_value->gettable(_lua_state, -3);

  // increment reference
  _api_value->pushinteger(_lua_state, 1);
  _api_value->arith(_lua_state, LUA_OPADD);

  _api_value->settable(_lua_state, -3);

  _api_stack->pop(_lua_state, 1);
}

void function_var::_decrement_reference(){
  if(!_luafunc_pointer)
    return;

  // get metadata s+1
  _push_metadata_table();

  _api_value->pushstring(_lua_state, FUNCTION_METADATA_REFERENCE_COUNT);
  _api_stack->pushvalue(_lua_state, -1); // copy for later use
  _api_value->gettable(_lua_state, -3);

  // decrement reference
  _api_value->pushinteger(_lua_state, 1);
  _api_value->arith(_lua_state, LUA_OPSUB);

  _api_value->settable(_lua_state, -3);

  _api_stack->pop(_lua_state, 1);
}

int function_var::_get_reference_count(){
  if(!_luafunc_pointer)
    return 0;

  // get metadat s+1
  _push_metadata_table();

  _api_value->pushstring(_lua_state, TABLE_METADATA_REFERENCE_COUNT);
  _api_value->gettable(_lua_state, -2);

  int _res = _api_value->tointeger(_lua_state, -1);
  _api_stack->pop(_lua_state, 2);

  return _res;
}


bool function_var::_has_reference_metadata(){
  if(!_luafunc_pointer)
    return false;

  return pstack_call<bool>(_lua_state, 0, 0, _api_stack, _api_value, [](function_var* _this){
    _this->_require_global_table();
    if(_this->_api_value->type(_this->_lua_state, -1) != LUA_TTABLE)
      return false;

    std::string _pkey = function_var::_get_reference_key(_this->_luafunc_pointer);
    _this->_api_value->pushstring(_this->_lua_state, _pkey.c_str());
    _this->_api_value->gettable(_this->_lua_state, -2);

    return _this->_api_value->type(_this->_lua_state, -1) == LUA_TTABLE;
  }, this);
}


std::string function_var::_get_reference_key(const void* pointer){
  return format_str("0x%X", (unsigned long long)pointer);
}


int function_var::_cb_entry_point(lua_State* state){
  if(lua_type(state, lua_upvalueindex(LUA_FUNCVAR_FUNCTION_UPVALUE)) != LUA_TLIGHTUSERDATA){
    lua_pushstring(state, "[function_var] Upvalue for actual function is not a pointer to a function.");
    lua_error(state);
  }

  luaapi_cfunction _actual_cb = (luaapi_cfunction)lua_touserdata(state, lua_upvalueindex(LUA_FUNCVAR_FUNCTION_UPVALUE));
  return _actual_cb(state, cpplua_get_api_compilation_context());
}


int function_var::_compare_with(const variant* rhs) const{
  function_var* _rhs = (function_var*)rhs;
  if(_this_fn == _rhs->_this_fn)
    return 0;

  return _this_fn < _rhs->_this_fn? -1: 1;
}


int function_var::get_type() const{
  return LUA_TFUNCTION;
}


function_var* function_var::create_copy_static(const I_function_var* data){
  function_var* _res;
  if(data->is_cfunction())
    _res = new function_var(data->get_function(), data->get_arg_closure());
  else{
    // assuming that there are already reference for this function
    _res = new function_var();

    _res->_api_stack = data->get_lua_stack_api_definition();
    _res->_api_value = data->get_lua_value_api_definition();

    _res->_lua_state = data->get_lua_interface();
    _res->_luafunc_pointer = data->get_lua_function();

    _res->_increment_reference();
  }

  return _res;
}


bool function_var::from_state(lua_State* state, int stack_idx){
  clear_function();

  int _type = lua_type(state, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str("Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lua_typename(state, _type),
      lua_typename(state, get_type())
    ).c_str());

    return false;
  }

  if(!lua_iscfunction(state, stack_idx)){
    _api_stack = cpplua_get_api_stack_definition();
    _api_value = cpplua_get_api_value_definition();

    _lua_state = state;
    _luafunc_pointer = lua_topointer(state, stack_idx);

    if(_has_reference_metadata())
      _increment_reference();
    else
      _create_reference();
  }
  else{
    _init_cfunction();
    int _func_idx = stack_idx < 0? LUA_CONV_T2B(state, stack_idx): stack_idx; 

    // Get actual function
    if(lua_getupvalue(state, _func_idx, LUA_FUNCVAR_FUNCTION_UPVALUE)){
      if(lua_type(state, -1) == LUA_TLIGHTUSERDATA)
        _this_fn = (luaapi_cfunction)lua_touserdata(state, -1);

      lua_pop(state, 1);
    }

    _fn_args->clear();
    int _idx = LUA_FUNCVAR_START_UPVALUE;
    while(_idx <= LUAI_MAXSTACK){
      if(!lua_getupvalue(state, _func_idx, _idx))
        break;

      variant* _var = to_variant(state, -1);
      _fn_args->append_var(_var);
      cpplua_delete_variant(_var);

      lua_pop(state, 1);
      _idx++;
    }
  }

  return true;
}

void function_var::push_to_stack(lua_State* state) const{
  if(is_cfunction()){
    if(!_this_fn){
      lua_pushnil(state);
      return;
    }

    // push user function as upvalue
    lua_pushlightuserdata(state, _this_fn);

    // END OF LUA_FUNCVAR_START_UPVALUE

    // push the upvalues first
    for(int i = 0; i < _fn_args->get_var_count(); i++)
      _fn_args->get_self_var(i)->push_to_stack(state);

    lua_pushcclosure(state, _cb_entry_point, LUA_FUNCVAR_START_UPVALUE_IDX+_fn_args->get_var_count());
  }
  else
    _push_actual_function();
}

variant* function_var::create_copy() const{
  return create_copy_static(this);
}


std::string function_var::to_string() const{
  if(is_cfunction())
    return format_str("CFunction 0x%X", (unsigned long long)_this_fn);
  else
    return format_str("LuaFunction 0x%X", (unsigned long long)_luafunc_pointer);
}

void function_var::to_string(I_string_store* pstring) const{
  pstring->append(to_string().c_str());
}


I_vararr* function_var::get_arg_closure(){
  return _fn_args;
}

const I_vararr* function_var::get_arg_closure() const{
  return _fn_args;
}


function_var::luaapi_cfunction function_var::get_function() const{
  return _this_fn;
}

bool function_var::set_function(luaapi_cfunction fn){
  if(_luafunc_pointer)
    return false;

  _this_fn = fn;
  return true;
}


bool function_var::is_cfunction() const{
  return _luafunc_pointer == NULL;
}


const void* function_var::get_lua_function() const{
  return _luafunc_pointer;
}


void function_var::clear_function(){
  if(_luafunc_pointer){
    if(_get_reference_count() > 1)
      _decrement_reference();
    else
      _delete_reference();

    _luafunc_pointer = NULL;
  }

  if(_fn_args){
    delete _fn_args;

    _this_fn = NULL;
    _fn_args = NULL;
  }
}


void* function_var::get_lua_interface() const{
  return _lua_state;
}


lua::api::I_stack* function_var::get_lua_stack_api_definition() const{
  return _api_stack;
}

lua::api::I_value* function_var::get_lua_value_api_definition() const{
  return _api_value;
}




// MARK: error_var def

error_var::error_var(){
  _err_data = new variant();
}

error_var::error_var(const error_var& var){
  _err_data = var._err_data? var._err_data->create_copy(): NULL;
  _error_code = var._error_code;
}

error_var::error_var(const variant* data, long long code){
  _err_data = data->create_copy();
  _error_code = code;
}

error_var::error_var(lua_State* state, int stack_idx){
  from_state(state, stack_idx);
}

error_var::~error_var(){
  if(_err_data)
    delete _err_data;
}


int error_var::get_type() const{
  return LUA_TERROR;
}


error_var* error_var::create_copy_static(const I_error_var* data){
  const I_variant* _idata = data->get_error_data_interface();
  variant* _new_data = cpplua_create_var_copy(_idata);

  error_var* _result = new error_var(_new_data, data->get_error_code());
  delete _new_data;

  return _result;
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


void error_var::to_string(I_string_store* pstring) const{
  std::string _str = to_string();
  pstring->append(_str.c_str());
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

const I_variant* error_var::get_error_data_interface() const{
  return _err_data;
}


long long error_var::get_error_code() const{
  return _error_code;
}

void error_var::set_error_code(int code){
  _error_code = code;
}




// MARK: object_var def

object_var::object_var(){}

object_var::object_var(I_object* obj_ref){
  _obj = obj_ref;
}


int object_var::get_type() const{
  return LUA_TCPPOBJECT;
}


object_var* object_var::create_copy_static(const I_object_var* data){
  return new object_var(data->get_object_reference());
}


bool object_var::from_state(lua_State* state, int stack_idx){
  I_object* _test_obj = object::get_obj_from_table(state, stack_idx);
  if(!_test_obj)
    return false;

  _obj = _test_obj;
  return true;
}

void object_var::push_to_stack(lua_State* state) const{
  if(!_obj)
    return;

  // for the object
  lua_newtable(state);

  object::push_object_to_table(state, _obj, -1);
}

variant* object_var::create_copy() const{
  return new object_var(_obj);
}


#define OBJECT_VAR_STRING_VAL "Cpp-Object (Table)"

void object_var::to_string(I_string_store* pstring) const{
  pstring->append(OBJECT_VAR_STRING_VAL);
}

std::string object_var::to_string() const{
  return OBJECT_VAR_STRING_VAL;
}


I_object* object_var::set_object_reference(I_object* object){
  I_object* _prev_obj = _obj;
  _obj = object;

  return _prev_obj;
}

I_object* object_var::get_object_reference() const{
  return _obj;
}




// MARK: comparison_variant def

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




// MARK: to_variant

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

    break; case function_var::get_static_lua_type():{
      _result = new function_var(state, stack_idx);
    }

    break; default:{
      _result = new variant();
    }
  }

  return _result;
}


// MARK: to_variant_fglobal

variant* lua::to_variant_fglobal(lua_State* state, const char* global_name){
  lua_getglobal(state, global_name);
  variant* _result = lua::to_variant(state, -1);
  lua_pop(state, 1);

  return _result;
}

// MARK: to_variant_ref

variant* lua::to_variant_ref(lua_State* state, int stack_idx){
  const void* _pointer_ref = lua_topointer(state, stack_idx);
  string_var* _var_res = new string_var(format_str("p_ref 0x%X", _pointer_ref));

  return _var_res;
}


// MARK: from_pointer_to_str

string_var lua::from_pointer_to_str(void* pointer){
  long long _pointer_num = (long long)pointer;
  return string_var(reinterpret_cast<const char*>(_pointer_num), sizeof(long long));
}


// MARK: from_str_to_pointer

void* lua::from_str_to_pointer(const string_var& str){
  long long _pointer_num = *(reinterpret_cast<const long long*>(str.get_string()));
  return (void*)_pointer_num;
}


// MARK: Static functions

lua::variant* cpplua_create_var_copy(const lua::I_variant* data){
  switch(data->get_type()){
    break; case string_var::get_static_lua_type():
      return string_var::create_copy_static(dynamic_cast<const I_string_var*>(data));

    break; case number_var::get_static_lua_type():
      return number_var::create_copy_static(dynamic_cast<const I_number_var*>(data));

    break; case bool_var::get_static_lua_type():
      return bool_var::create_copy_static(dynamic_cast<const I_bool_var*>(data));

    break; case table_var::get_static_lua_type():
      return table_var::create_copy_static(dynamic_cast<const I_table_var*>(data));

    break; case lightuser_var::get_static_lua_type():
      return lightuser_var::create_copy_static(dynamic_cast<const I_lightuser_var*>(data));

    break; case error_var::get_static_lua_type():
      return error_var::create_copy_static(dynamic_cast<const I_error_var*>(data));

    break; case object_var::get_static_lua_type():
      return object_var::create_copy_static(dynamic_cast<const I_object_var*>(data));

    break; case function_var::get_static_lua_type():
      return function_var::create_copy_static(dynamic_cast<const I_function_var*>(data));

    break; default:
      return new nil_var();
  }
}


// MARK: DLL functions

DLLEXPORT void CPPLUA_VARIANT_SET_DEFAULT_LOGGER(I_logger* _logger){
  set_default_logger(_logger);
}

DLLEXPORT void CPPLUA_DELETE_VARIANT(const lua::I_variant* data){
  delete data;
}

DLLEXPORT const char* CPPLUA_GET_TYPE_NAME(int type_name){
  switch(type_name){
    break; case LUA_TNIL: return "NIL";
    break; case LUA_TNUMBER: return "Number";
    break; case LUA_TBOOLEAN: return "Bool";
    break; case LUA_TSTRING: return "String";
    break; case LUA_TTABLE: return "Table";
    break; case LUA_TFUNCTION: return "Function";
    break; case LUA_TUSERDATA: return "Lua-Userdata";
    break; case LUA_TTHREAD: return "Lua-Thread";
    break; case LUA_TLIGHTUSERDATA: return "Lua-Light-Userdata";
    break; default: return "UNK";
  }
}



class _api_variant_util_definition: public lua::api::I_variant_util{
  public:
    I_variant* to_variant(void* istate, int stack_idx) override{return lua::to_variant((lua_State*)istate, stack_idx);}
    I_variant* to_variant_fglobal(void* istate, const char* name) override{return lua::to_variant_fglobal((lua_State*)istate, name);}
    I_variant* to_variant_ref(void* istate, int stack_idx) override{return lua::to_variant_ref((lua_State*)istate, stack_idx);}

    void push_variant(void* istate, const I_variant* var) override{dynamic_cast<const variant*>(var)->push_to_stack((lua_State*)istate);}

    I_variant* create_variant_copy(const I_variant* var) override{return cpplua_create_var_copy(var);}
    void delete_variant(const I_variant* var) override{cpplua_delete_variant(var);}
};


static _api_variant_util_definition __api_def;

DLLEXPORT lua::api::I_variant_util* CPPLUA_GET_API_VARIANT_UTIL_DEFINITION(){
  return &__api_def;
}