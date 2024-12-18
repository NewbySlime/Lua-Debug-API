#include "dllutil.h"
#include "luaapi_debug.h"
#include "luaapi_execution.h"
#include "luaapi_object_util.h"
#include "luaapi_state.h"
#include "luaapi_util.h"
#include "luaapi_table_util.h"
#include "luaapi_variant_util.h"
#include "luacpp_error_handling.h"
#include "luamemory_util.h"
#include "luaobject_util.h"
#include "luastack_iter.h"
#include "luatable_util.h"
#include "luautility.h"
#include "luavariant_arr.h"
#include "luavariant_util.h"
#include "luavariant.h"
#include "std_logger.h"
#include "string_util.h"

#include "algorithm"
#include "cmath"
#include "cstring"
#include "thread"
#include "set"

// disable min macro
#ifdef min
#undef min
#endif

#define BASE_VARIANT_TYPE_NAME "nil"

#define NUM_VAR_EQUAL_ROUND_COMP 0.01

#define TABLE_REFERENCE_INTERNAL_DATA "__clua_table_ref_data"
#define FUNCTION_REFERENCE_INTERNAL_DATA "__clua_func_ref_data"
#define OBJECT_REFERENCE_INTERNAL_DATA "__clua_obj_ref_data"

#ifndef FUNCTION_LUA_COPY_STRIP_DBG
#define FUNCTION_LUA_COPY_STRIP_DBG true
#endif


using namespace dynamic_library::util;
using namespace lua;
using namespace lua::api;
using namespace lua::memory;
using namespace lua::utility;
using namespace ::memory;


static const I_dynamic_management* __dm = get_memory_manager();

std_logger _default_logger = std_logger();
const I_logger* _logger = &_default_logger;


void _manual_code_initialize(){
  if(!__dm)
    __dm = get_memory_manager();
}



// MARK: set_default_logger

void lua::set_default_logger(I_logger* logger){
  _logger = logger;
}




// MARK: variant def

variant::variant(){
  _manual_code_initialize();
}


int variant::get_type() const{
  return LUA_TNIL;
}


void variant::push_to_stack(const core* lc) const{
  lc->context->api_value->pushnil(lc->istate);
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




// MARK: string_var def

string_var::string_var(){
  _init_class();
}

string_var::string_var(const string_var& var){
  _init_class();
  _copy_from_var(&var);
}

string_var::string_var(const I_string_var* var){
  _init_class();
  _copy_from_var(var);
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

string_var::string_var(const core* lc, int stack_idx){
  _init_class();
  from_state(lc, stack_idx);
}

string_var::~string_var(){
  __dm->free(_str_mem, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}


void string_var::_init_class(){
  _mem_size = 0;
  _str_mem = (char*)__dm->malloc(_mem_size+1, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  _put_delimiter_at_end();
}

void string_var::_copy_from_var(const I_string_var* var){
  _set_to_cstr(var->get_string(), var->get_length());
}


void string_var::_put_delimiter_at_end(){
  _str_mem[_mem_size] = '\0';
}


void string_var::_resize_mem(std::size_t newlen){
  _str_mem = (char*)__dm->realloc(_str_mem, newlen+1, DYNAMIC_MANAGEMENT_DEBUG_DATA);
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


bool string_var::from_state(const core* lc, int stack_idx){
  stack_idx = lc->context->api_stack->absindex(lc->istate, stack_idx);
  int _type = lc->context->api_value->type(lc->istate, stack_idx);
  switch(_type){
    break;
      case LUA_TSTRING:
      case LUA_TNUMBER:
    
    break; default:{
      _logger->print_error(format_str_mem(__dm, "Mismatched type when converting at stack (%d). (%s-%s)\n",
        stack_idx,
        lc->context->api_value->ttypename(lc->istate, _type),
        lc->context->api_value->ttypename(lc->istate, get_type())
      ).c_str());

      return false;
    }
  }

  std::size_t _str_len;
  const char* _cstr = lc->context->api_value->tolstring(lc->istate, stack_idx, &_str_len);
  _set_to_cstr(_cstr, _str_len);

  return true;
}

void string_var::push_to_stack(const core* lc) const{
  lc->context->api_value->pushlstring(lc->istate, _str_mem, _mem_size);
}


void string_var::to_string(I_string_store* pstring) const{
  return pstring->append(_str_mem, _mem_size);
}

std::string string_var::to_string() const{
  return std::string(_str_mem, _mem_size);
}


string_var& string_var::operator=(const string_var& data){
  _set_to_cstr(data.get_string(), data.get_length());
  return *this;
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

number_var::number_var(const number_var& var){
  _copy_from_var(&var);
}

number_var::number_var(const I_number_var* var){
  _copy_from_var(var);
}

number_var::number_var(const double& from){
  _this_data = from;
}

number_var::number_var(const core* lc, int stack_idx){
  from_state(lc, stack_idx);
}


void number_var::_copy_from_var(const I_number_var* var){
  _this_data = var->get_number();
}


int number_var::_comp_num(const number_var& var1, const number_var& var2){
  double _delta_var = var1._this_data - var2._this_data;
  
  // equal comparison
  if(abs(_delta_var) <= NUM_VAR_EQUAL_ROUND_COMP)
    return 0;
  
  return var1._this_data < var2._this_data? -1: 1;
}


int number_var::_compare_with(const variant* rhs) const{
  return _comp_num(*this, *(number_var*)rhs);
}


int number_var::get_type() const{
  return LUA_TNUMBER;
}


bool number_var::from_state(const core* lc, int stack_idx){
  stack_idx = lc->context->api_stack->absindex(lc->istate, stack_idx);
  _this_data = 0;

  int _type = lc->context->api_value->type(lc->istate, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str_mem(__dm, "Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lc->context->api_value->ttypename(lc->istate, _type),
      lc->context->api_value->ttypename(lc->istate, get_type())
    ).c_str());

    return false;
  }

  _this_data = lc->context->api_value->tonumber(lc->istate, stack_idx);
  return true;
}

void number_var::push_to_stack(const core* lc) const{
  lc->context->api_value->pushnumber(lc->istate, _this_data);
}


void number_var::to_string(I_string_store* data) const{
  std::string _str_data = to_string();
  data->append(_str_data.c_str());
}

std::string number_var::to_string() const{
  int _str_len = snprintf(NULL, 0, "%f", _this_data);
  
  char* _c_str = (char*)__dm->malloc(_str_len+1, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  snprintf(_c_str, _str_len+1, "%f", _this_data);

  std::string _result = _c_str;
  __dm->free(_c_str, DYNAMIC_MANAGEMENT_DEBUG_DATA);

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

bool_var::bool_var(const bool_var& var){
  _copy_from_var(&var);
}

bool_var::bool_var(const I_bool_var* var){
  _copy_from_var(var);
}

bool_var::bool_var(const bool& from){
  _this_data = from;
}

bool_var::bool_var(const core* lc, int stack_idx){
  from_state(lc, stack_idx);
}


void bool_var::_copy_from_var(const I_bool_var* data){
  _this_data = data->get_boolean();
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


bool bool_var::from_state(const core* lc, int stack_idx){
  stack_idx = lc->context->api_stack->absindex(lc->istate, stack_idx);
  _this_data = true;

  int _type = lc->context->api_value->type(lc->istate, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str_mem(__dm, "Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lc->context->api_value->ttypename(lc->istate, _type),
      lc->context->api_value->ttypename(lc->istate, get_type())
    ).c_str());

    return false;
  }

  _this_data = lc->context->api_value->toboolean(lc->istate, stack_idx);
  return true;
}

void bool_var::push_to_stack(const core* lc) const{
  lc->context->api_value->pushboolean(lc->istate, _this_data);
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

static void _table_var_code_initiate();
static void _table_var_code_deinitiate();
static destructor_helper _table_var_dh(_table_var_code_deinitiate);

static void _table_var_code_initiate(){
  if(!_sr_data_map)
    _sr_data_map = __dm->new_class_dbg<std::map<std::thread::id, _self_reference_data*>>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

static void _table_var_code_deinitiate(){
  if(_sr_data_map)
    __dm->delete_class_dbg(_sr_data_map, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}


// Create copy of a table
table_var::table_var(){
  _init_class();

  _table_data = __dm->new_class_dbg<std::map<comparison_variant, variant*>>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
  _is_ref = false;
}

// Copy attributes from a table variant
table_var::table_var(const table_var& var){
  _init_class();
  _copy_from_var(&var);
}

table_var::table_var(const I_table_var* var){
  _init_class();
  _copy_from_var(var);
}

// Create a reference
table_var::table_var(const core* lc, int stack_idx){
  _init_class();
  from_state(lc, stack_idx);
}

table_var::table_var(const object_var& var){
  _init_class();
  from_object(&var);
}

table_var::table_var(const I_object_var* var){
  _init_class();
  from_object(var);
}


table_var::~table_var(){
  _clear_table();
  __dm->free(_keys_buffer, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}


void table_var::_init_class(){
  _table_var_code_initiate();
  _init_keys_reg();
}


void table_var::_init_table_data(){
  if(_table_data)
    return;

  _table_data = __dm->new_class_dbg<std::map<comparison_variant, variant*>>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

void table_var::_clear_table_data(){
  if(!_table_data)
    return;

  for(auto _iter: *_table_data)
    __dm->delete_class_dbg(_iter.second, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  _table_data->clear();
  _update_keys_reg();
}

void table_var::_clear_table_reference_data(){
  if(!_tref)
    return;

  _update_keys_reg();

  // table value s+1
  _tref->push_value_to_stack();

  int _idx = 0;
  while(_keys_buffer[_idx]){
    const I_variant* _key = _keys_buffer[_idx];
    _key->push_to_stack(&_lc);
    _lc.context->api_value->pushnil(_lc.istate); // removing by setting to nil
    _lc.context->api_value->settable(_lc.istate, -3);

    _idx++;
  }

  _lc.context->api_stack->pop(_lc.istate, 1); // pop table value

  _update_keys_reg();
}

void table_var::_clear_table(){
  if(_tref){ // For reference, only dereferencing. Don't remove values inside reference.
    __dm->delete_class_dbg(_tref, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _tref = NULL;
    _table_pointer = NULL;
  }

  if(_table_data){
    _clear_table_data();
    __dm->delete_class_dbg(_table_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _table_data = NULL;
  }

  _clear_keys_reg();

  _is_ref = false;
}


void table_var::_copy_from_var(const I_table_var* data){
  _clear_table();

  _is_ref = data->is_reference();
  if(_is_ref){
    // assume that there are already reference metadata, and we only need to increment the reference
    _table_pointer = data->get_table_pointer();
    _lc = *data->get_lua_core();

    _tref = __dm->new_class_dbg<value_ref>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_lc, TABLE_REFERENCE_INTERNAL_DATA, _get_reference_key(_table_pointer).c_str());
    _update_keys_reg();
  }
  else{
    _init_table_data();

    const I_variant** _key_arr = data->get_keys();

    int _idx = 0;
    while(_key_arr[_idx]){
      const I_variant* _key_data = _key_arr[_idx];
      const I_variant* _value_data = data->get_value(_key_data);
      set_value(_key_data, _value_data);

      data->free_variant(_value_data);
      _idx++;
    }
  }
}


std::string table_var::_get_reference_key(const void* pointer){
  return format_str_mem(__dm, "0x%X", (unsigned long long)pointer);
}


void table_var::_fs_iter_cb(const lua::api::core* lc, int key_stack, int val_stack, int iter_idx, void* cb_data){
  int _i_debug = 0;

  table_var* _this = (table_var*)cb_data;

  int _type = lc->context->api_varutil->get_special_type(lc->istate, key_stack);
  I_variant* _key_value;
  switch(_type){
    break;
      case LUA_TSTRING:
      case LUA_TNUMBER:
      case LUA_TNIL:
      case LUA_TLIGHTUSERDATA:
      case LUA_TUSERDATA:
      case LUA_TBOOLEAN:{
        _key_value = lc->context->api_varutil->to_variant(lc->istate, key_stack);
      }

    break; default:{
      _key_value = lc->context->api_varutil->to_variant_ref(lc->istate, key_stack);
    }
  }

  I_variant* _value = lc->context->api_varutil->to_variant(lc->istate, val_stack);
  if(_value->get_type() == LUA_TNIL){
    string_store _str;
    _key_value->to_string(&_str);
    _logger->print_warning(format_str_mem(__dm, "Value of (type %d %s) is Nil. Is this intended?\n", _key_value->get_type(), _str.data.c_str()).c_str());
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
      _logger->print_error(format_str_mem(__dm, "Using type (%s) as key is not supported.\n",
        lc->context->api_value->ttypename(lc->istate, _key_value->get_type())
      ).c_str());
    }
  }

  lc->context->api_varutil->delete_variant(_key_value);
  lc->context->api_varutil->delete_variant(_value);

  return;
}


variant* table_var::_get_value_by_interface(const I_variant* key) const{
  if(!_tref)
    return NULL;

  // get actual table s+1
  _tref->push_value_to_stack();

  // get value s+1
  key->push_to_stack(&_lc);
  int _type = _lc.context->api_value->gettable(_lc.istate, -2);
  variant* _res = NULL;
  if(_type != LUA_TNIL){
    // create compilation specific copy
    // TODO need optimization
    I_variant* _res_tmp = _lc.context->api_varutil->to_variant(_lc.istate, -1);
    _res = cpplua_create_var_copy(_res_tmp);
    _lc.context->api_varutil->delete_variant(_res_tmp);
  }

  _lc.context->api_stack->pop(_lc.istate, 2);
  return _res;
}

void table_var::_set_value_by_interface(const I_variant* key, const I_variant* value){
  if(!_tref)
    return;

  // get actual table s+1
  _tref->push_value_to_stack();

  // set value
  key->push_to_stack(&_lc);
  value->push_to_stack(&_lc);
  _lc.context->api_value->settable(_lc.istate, -3);

  _lc.context->api_stack->pop(_lc.istate, 1);

  _update_keys_reg();
}

bool table_var::_remove_value_by_interface(const I_variant* key){
  if(!_tref)
    return false;

  // get actual table s+1
  _tref->push_value_to_stack();

  // set value to nil
  key->push_to_stack(&_lc);
  _lc.context->api_value->pushnil(_lc.istate);
  _lc.context->api_value->settable(_lc.istate, -3);

  _lc.context->api_stack->pop(_lc.istate, 1);

  _update_keys_reg();
  return true;
}


variant* table_var::_get_value(const comparison_variant& comp_var) const{
  if(!_table_data)
    return NULL;

  auto _iter = _table_data->find(comp_var);
  if(_iter == _table_data->end())
    return NULL;

  return cpplua_create_var_copy(_iter->second);
}

variant* table_var::_get_value(const I_variant* key) const{
  if(!_table_data)
    return NULL;

  variant* _key_data = cpplua_create_var_copy(key);
  comparison_variant _key_comp(_key_data); __dm->delete_class_dbg(_key_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  
  return _get_value(_key_comp);
}


void table_var::_set_value(const comparison_variant& comp_key, const variant* value){
  if(!_table_data)
    _init_table_data();

  _remove_value(comp_key);
  _table_data->operator[](comp_key) = cpplua_create_var_copy(value);

  _update_keys_reg();
}

void table_var::_set_value(const I_variant* key, const I_variant* value){
  variant* _key_data = cpplua_create_var_copy(key);
  comparison_variant _key_comp(_key_data);

  variant* _value_data = cpplua_create_var_copy(value);
  _set_value(_key_comp, _value_data);

  __dm->delete_class_dbg(_key_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  __dm->delete_class_dbg(_value_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}


bool table_var::_remove_value(const comparison_variant& comp_key){
  if(!_table_data)
    return false;

  auto _check_iter = _table_data->find(comp_key);
  if(_check_iter == _table_data->end())
    return false;

  __dm->delete_class_dbg(_check_iter->second, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  _table_data->erase(comp_key);

  _update_keys_reg();
  return true;
}

bool table_var::_remove_value(const I_variant* key){
  if(!_table_data)
    return false;

  variant* _key_data = cpplua_create_var_copy(key);
  comparison_variant _key_comp(_key_data); __dm->delete_class_dbg(_key_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);

  return _remove_value(_key_comp);
}


void table_var::_init_keys_reg(){
  _keys_buffer = (const I_variant**)__dm->malloc(sizeof(I_variant*), DYNAMIC_MANAGEMENT_DEBUG_DATA);
  _keys_buffer[0] = NULL;
}

void table_var::_update_keys_reg(){
  _clear_keys_reg();

  if(_is_ref){
    if(!_tref)
      return;

    // get actual table s+1
    _tref->push_value_to_stack();

    int _key_len = _lc.context->api_tableutil->table_len(_lc.istate, -1);
    _keys_buffer = (const I_variant**)__dm->realloc(_keys_buffer, (_key_len+1) * sizeof(I_variant*), DYNAMIC_MANAGEMENT_DEBUG_DATA);

    struct iter_data{
      table_var* _this;
      int current_idx;
    }; iter_data _iterd;

    _iterd._this = this;
    _iterd.current_idx = 0;

    _lc.context->api_tableutil->iterate_table(_lc.istate, -1, [](const core* lc, int key_stack, int val_stack, int iter_idx, void* cb_data){
      iter_data* _iterd = (iter_data*)cb_data;
      const core& _lc = *lc;
      
      I_variant* _ivar = _lc.context->api_varutil->to_variant(_lc.istate, key_stack);
      _iterd->_this->_keys_buffer[_iterd->current_idx] = cpplua_create_var_copy(_ivar);
      _lc.context->api_varutil->delete_variant(_ivar);

      _iterd->current_idx++;
    }, &_iterd);
    
    _lc.context->api_stack->pop(_lc.istate, 1);
    _keys_buffer[_key_len] = NULL;
  }
  else{
    _keys_buffer = (const I_variant**)__dm->realloc(_keys_buffer, (_table_data->size()+1) * sizeof(I_variant*), DYNAMIC_MANAGEMENT_DEBUG_DATA);

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
    __dm->delete_class_dbg(_keys_buffer[_idx], DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _idx++;
  }

  _keys_buffer = (const I_variant**)__dm->realloc(_keys_buffer, sizeof(I_variant*), DYNAMIC_MANAGEMENT_DEBUG_DATA);
  _keys_buffer[0] = NULL;
}


int table_var::_compare_with(const variant* rhs) const{
  table_var* _rhs = (table_var*)rhs;
  if(_is_ref != _rhs->_is_ref)
    return _is_ref > _rhs->_is_ref? 1: -1;

  if(!_is_ref){
    if(this == _rhs)
      return 0;
    
    return this > _rhs? 1: -1;
  }

  if(_table_pointer == _rhs->_table_pointer)
    return 0;

  return _table_pointer > _rhs->_table_pointer? 1: -1;
}


int table_var::get_type() const{
  return LUA_TTABLE;
}


bool table_var::from_state(const core* lc, int stack_idx){
  stack_idx = lc->context->api_stack->absindex(lc->istate, stack_idx);
  _clear_table();

  // use the core in the argument first
  int _type = lc->context->api_value->type(lc->istate, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str_mem(__dm, "Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lc->context->api_value->ttypename(lc->istate, _type),
      lc->context->api_value->ttypename(lc->istate, get_type())
    ).c_str());

    return false;
  }

  // use copy of core
  _lc = *lc;
  _table_pointer = _lc.context->api_value->topointer(_lc.istate, stack_idx);

  _tref = __dm->new_class_dbg<value_ref>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_lc, TABLE_REFERENCE_INTERNAL_DATA, _get_reference_key(_table_pointer).c_str());
  if(!_tref->reference_initiated())
    _tref->set_value(stack_idx);

  _is_ref = true;

  _update_keys_reg();

  return true;
}

bool table_var::from_state_copy(const core* lc, int stack_idx){
  stack_idx = lc->context->api_stack->absindex(lc->istate, stack_idx);
  _clear_table();
  _table_data = __dm->new_class_dbg<std::map<comparison_variant, variant*>>(DYNAMIC_MANAGEMENT_DEBUG_DATA);

  // never copy core to itself, as this does not use referencing
  int _type = lc->context->api_value->type(lc->istate, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str_mem(__dm, "Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lc->context->api_value->ttypename(lc->istate, _type),
      lc->context->api_value->ttypename(lc->istate, get_type())
    ).c_str());

    return false;
  }

  bool _has_sr_ownership = false;
  _self_reference_data* _sr_data;
  auto _iter = _sr_data_map->find(std::this_thread::get_id());
  if(_iter == _sr_data_map->end()){
    _has_sr_ownership = true;

    _sr_data = __dm->new_class_dbg<_self_reference_data>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _sr_data_map->operator[](std::this_thread::get_id()) = _sr_data;
  }
  else
    _sr_data = _iter->second;

  const void *_this_luapointer = lc->context->api_value->topointer(lc->istate, stack_idx);
  if(_sr_data->referenced_data.find(_this_luapointer) != _sr_data->referenced_data.end())
    goto skip_parsing_label;

  _sr_data->referenced_data.insert(_this_luapointer);
  lc->context->api_tableutil->iterate_table(lc->istate, stack_idx, _fs_iter_cb, this);

  _is_ref = false;
  
  skip_parsing_label:{
    if(_has_sr_ownership){
      _sr_data_map->erase(std::this_thread::get_id());
      __dm->delete_class_dbg(_sr_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    }
  }

  return true;
}

bool table_var::from_object(const I_object_var* obj){
  const core* _lc = obj->get_lua_core();

  obj->push_to_stack(_lc);
  bool _res = from_state(_lc, -1);
  _lc->context->api_stack->pop(_lc->istate, 1);

  return _res;
}

void table_var::push_to_stack(const core* lc) const{
  if(_is_ref){
    // Check if target state is in the same state where the reference are put
    if(check_same_runtime(&_lc, lc))
      _tref->push_value_to_stack();
    else // prohibit if the state address (in memory) is not the same
      push_to_stack_copy(lc);
  }
  else{
    lc->context->api_value->newtable(lc->istate);
    for(auto _iter: *_table_data)
      lc->context->api_tableutil->set_table_value(lc->istate, -1, _iter.first.get_comparison_data(), _iter.second);
  }
}

void table_var::push_to_stack_copy(const core* lc) const{
  // Use push_to_stack if this already has the copy
  if(!_is_ref){
    push_to_stack(lc);
    return;
  }

  table_var* _tmpvar = dynamic_cast<table_var*>(cpplua_create_var_copy(this));
  _tmpvar->as_copy();
  _tmpvar->push_to_stack(lc);
  cpplua_delete_variant(_tmpvar);
}


#define TABLE_VAR_TO_STRING() "table"

void table_var::to_string(I_string_store* pstring) const{
  pstring->append(TABLE_VAR_TO_STRING());
}

std::string table_var::to_string() const{
  return TABLE_VAR_TO_STRING();
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


void table_var::clear_table(){
  // Clear both, just in case
  _clear_table_data();
  _clear_table_reference_data();
}


void table_var::as_copy(){
  if(!_is_ref) // already a copy
    return;

  push_to_stack(&_lc);
  from_state_copy(&_lc, -1);
  _lc.context->api_stack->pop(_lc.istate, 1);
}


bool table_var::is_reference() const{
  return _is_ref;
}

const void* table_var::get_table_pointer() const{
  return _table_pointer;
}

const core* table_var::get_lua_core() const{
  return &_lc;
}


void table_var::free_variant(const I_variant* var) const{
  cpplua_delete_variant(var);
}




// MARK: lightuser_var def

lightuser_var::lightuser_var(){
  _pointer_data = NULL;
}

lightuser_var::lightuser_var(const lightuser_var& data){
  _copy_from_var(&data);
}

lightuser_var::lightuser_var(const I_lightuser_var* data){
  _copy_from_var(data);
}

lightuser_var::lightuser_var(void* pointer){
  _pointer_data = pointer;
}

lightuser_var::lightuser_var(const core* lc, int stack_idx){
  from_state(lc, stack_idx);
}


void lightuser_var::_copy_from_var(const I_lightuser_var* data){
  _pointer_data = data->get_data();
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



bool lightuser_var::from_state(const core* lc, int stack_idx){
  stack_idx = lc->context->api_stack->absindex(lc->istate, stack_idx);
  int _type = lc->context->api_value->type(lc->istate, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str_mem(__dm, "Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lc->context->api_value->ttypename(lc->istate, _type),
      lc->context->api_value->ttypename(lc->istate, get_type())
    ).c_str());

    return false;
  }

  _pointer_data = lc->context->api_value->touserdata(lc->istate, stack_idx);
  return true;
}

void lightuser_var::push_to_stack(const core* lc) const{
  lc->context->api_value->pushlightuserdata(lc->istate, _pointer_data);
}


#define LIGHTUSER_VAR_TO_STRING(ptr) format_str_mem(__dm, "lud 0x%X", ptr)

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

function_var::function_var(){
  _init_class();
}

function_var::function_var(const function_var& var){
  _init_class();
  _copy_from_var(&var);
}

function_var::function_var(const I_function_var* var){
  _init_class();
  _copy_from_var(var);
}

function_var::function_var(luaapi_cfunction fn, const I_vararr* args){
  _init_class();
  reset_cfunction(fn, args);
}

function_var::function_var(const void* chunk_buf, size_t len, const char* chunk_name){
  _init_class();
  reset_luafunction(chunk_buf, len, chunk_name);
}

function_var::function_var(const core* lc, int stack_idx){
  _init_class();
  from_state(lc, stack_idx);
}

function_var::~function_var(){
  _clear_function_data();
}


void function_var::_init_class(){
  _init_cfunction();
}

void function_var::_init_cfunction(){
  if(_fn_args)
    return;

  _fn_args = __dm->new_class_dbg<vararr>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

void function_var::_clear_function_data(){
  if(_fn_args){
    __dm->delete_class_dbg(_fn_args, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _fn_args = NULL;
    _this_fn = NULL;
  }

  if(_fref){
    __dm->delete_class_dbg(_fref, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _fref = NULL;
    _func_pointer = NULL;
  }

  if(_fn_binary_data){
    __dm->free(_fn_binary_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _fn_binary_data = NULL;
    _fn_binary_data = 0;
  }

  if(_fn_binary_name){
    __dm->free(_fn_binary_name, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _fn_binary_name = NULL;
  }

  // Make a reset
  _is_cfunction = true;
  _is_reference = false;
}


void function_var::_copy_from_var(const I_function_var* data){
  _clear_function_data();

  _is_reference = data->is_reference();
  _is_cfunction = data->is_cfunction();

  // Create reference
  if(data->is_reference()){
    _lc = *data->get_lua_core();
    _func_pointer = data->get_lua_function_pointer();
    _fref = __dm->new_class_dbg<value_ref>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_lc, FUNCTION_REFERENCE_INTERNAL_DATA, _get_reference_key(_func_pointer).c_str());

    return;
  }

  // Create copy
  if(data->is_cfunction())
    reset_cfunction(data->get_function(), data->get_arg_closure());
  else
    reset_luafunction(data->get_function_binary_chunk(), data->get_function_binary_chunk_len());
}


std::string function_var::_get_reference_key(const void* pointer){
  return format_str_mem(__dm, "0x%X", (unsigned long long)pointer);
}


int function_var::_cb_entry_point(lua_State* state){
#ifdef LUA_CODE_EXISTS
  if(lua_type(state, lua_upvalueindex(LUA_FUNCVAR_FUNCTION_UPVALUE)) != LUA_TLIGHTUSERDATA){
    lua_pushstring(state, "[function_var] Upvalue for actual function is not a pointer to a function.");
    lua_error(state);
  }

  core _lc = as_lua_api_core(state);

  luaapi_cfunction _actual_cb = (luaapi_cfunction)lua_touserdata(state, lua_upvalueindex(LUA_FUNCVAR_FUNCTION_UPVALUE));
  return _actual_cb(&_lc);
#else
  return 0;
#endif
}


int function_var::_fn_chunk_data_writer(lua_State* state, const void* p, size_t sz, void* ud){
#ifdef LUA_CODE_EXISTS
  function_var* _fvar = (function_var*)ud;
  size_t _prev_size = _fvar->_fn_binary_len;
  size_t _new_size = _prev_size + sz;
  _fvar->_fn_binary_len = _new_size;
  _fvar->_fn_binary_data = __dm->realloc(_fvar->_fn_binary_data, _new_size, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  memcpy(&((char*)_fvar->_fn_binary_data)[_prev_size], p, sz);
  return 0;
#else
  return -1;
#endif
}


struct _chk_reader_data{
  const function_var* _this;
  int current_idx;
};

const char* function_var::_fn_chunk_data_reader(lua_State* state, void* data, size_t* size){
  const char* _result = NULL;
  *size = 0;

#ifdef LUA_CODE_EXISTS
  _chk_reader_data* _data = (_chk_reader_data*)data;
  if(_data->current_idx < _data->_this->_fn_binary_len){
    _result = &((char*)_data->_this->_fn_binary_data)[_data->current_idx];
    *size = _data->_this->_fn_binary_len - _data->current_idx;

    _data->current_idx += *size;
  }
#endif

  return _result;
}


int function_var::_compare_with(const variant* rhs) const{
  function_var* _rhs = (function_var*)rhs;
  if(_is_reference != _rhs->_is_reference)
    return _is_reference > _rhs->_is_reference? 1: -1;

  // return based on this pointer
  if(!_is_reference){
    if(this == _rhs)
      return 0;

    return this > _rhs? 1: -1;
  }

  if(_func_pointer == _rhs->_func_pointer)
    return 0;

  return _func_pointer > _rhs->_func_pointer? 1: -1;
}


int function_var::get_type() const{
  return LUA_TFUNCTION;
}


bool function_var::from_state(const core* lc, int stack_idx){
  stack_idx = lc->context->api_stack->absindex(lc->istate, stack_idx);
  _clear_function_data();

  int _type = lc->context->api_value->type(lc->istate, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str_mem(__dm, "Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lc->context->api_value->ttypename(lc->istate, _type),
      lc->context->api_value->ttypename(lc->istate, get_type())
    ).c_str());

    return false;
  }

  _is_cfunction = lc->context->api_value->iscfunction(lc->istate, stack_idx);
  _is_reference = true;

  _lc = *lc;
  _func_pointer = _lc.context->api_value->topointer(_lc.istate, stack_idx);

  _fref = __dm->new_class_dbg<value_ref>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_lc, FUNCTION_REFERENCE_INTERNAL_DATA, _get_reference_key(_func_pointer).c_str());
  if(!_fref->reference_initiated())
    _fref->set_value(stack_idx);

  return true;
}

bool function_var::from_state_copy(const core* lc, int stack_idx){
  stack_idx = lc->context->api_stack->absindex(lc->istate, stack_idx);
  _clear_function_data();

  int _type = lc->context->api_value->type(lc->istate, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str_mem(__dm, "Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lc->context->api_value->ttypename(lc->istate, _type),
      lc->context->api_value->ttypename(lc->istate, get_type())
    ).c_str());

    return false;
  }

  _is_cfunction = lc->context->api_value->iscfunction(lc->istate, stack_idx);
  _is_reference = false;

  if(_is_cfunction){
    _init_cfunction();

    // Create C function copy
    int _func_idx = lc->context->api_stack->convtop2bottom(lc->istate, stack_idx);

    // Get actual function
    if(
      lc->context->api_debug->getupvalue(lc->istate, _func_idx, LUA_FUNCVAR_FUNCTION_UPVALUE) == NULL ||
      lc->context->api_value->type(lc->istate, -1) != LUA_TLIGHTUSERDATA
    ){
      if(_logger){
        _logger->print_error(format_str_mem(__dm, "Function format is invalid (at idx %d). Creating copy will be skipped.", stack_idx).c_str());
      }

      return false;
    }

    _this_fn = (luaapi_cfunction)lc->context->api_value->touserdata(lc->istate, -1);
    lc->context->api_stack->pop(lc->istate, 1);

    int _idx = LUA_FUNCVAR_START_UPVALUE;
    while(_idx <= LUAI_MAXSTACK){
      if(!lc->context->api_debug->getupvalue(lc->istate, _func_idx, _idx))
        break;

      I_variant* _var = lc->context->api_varutil->to_variant(lc->istate, -1);
      _fn_args->append_var(_var);
      lc->context->api_varutil->delete_variant(_var);

      lc->context->api_stack->pop(lc->istate, 1);
      _idx++;
    }
  }
  else{
    // Trigger parse from lc's function_var compilation code
    bool _parse_from_lc_var = false;

#ifdef LUA_CODE_EXISTS
    // Check the version first
    if(verify_lua_version(cpplua_get_api_compilation_context(), lc->context)){
      // Prepare the buffer
      _fn_binary_data = __dm->malloc(0, DYNAMIC_MANAGEMENT_DEBUG_DATA);
      // Copy function for dump and getinfo (deleted at getinfo)
      lc->context->api_stack->pushvalue(lc->istate, stack_idx);
      lc->context->api_util->dump(lc->istate, _fn_chunk_data_writer, this, FUNCTION_LUA_COPY_STRIP_DBG);

      // Get function name (by debug functions)
      // CAREFUL: interface data
      lua_Debug* _idbg = (lua_Debug*)lc->context->api_debug->create_lua_debug_obj();
      lc->context->api_debug->getstack(lc->istate, 0, _idbg);

      lc->context->api_debug->getinfo(lc->istate, ">n", _idbg);
      size_t _nlen = _idbg->name? strlen(_idbg->name): 0;
      _fn_binary_name = (char*)__dm->malloc(_nlen+1, DYNAMIC_MANAGEMENT_DEBUG_DATA); // With null-termination character
      if(_idbg->name)
        memcpy(_fn_binary_name, _idbg->name, _nlen);
      _fn_binary_name[_nlen] = '\0';

      lc->context->api_debug->delete_lua_debug_obj(_idbg);
    }
    else
      _parse_from_lc_var = true;
#else
    _parse_from_lc_var = true;
#endif

    if(_parse_from_lc_var){
      I_function_var* _tmp_fvar = dynamic_cast<I_function_var*>(lc->context->api_varutil->create_variant_copy(this));
      _tmp_fvar->from_state_copy(lc, stack_idx);
      _copy_from_var(_tmp_fvar);
      lc->context->api_varutil->delete_variant(_tmp_fvar);
    }
  }

  return true;
}

void function_var::push_to_stack(const core* lc) const{
  // Is reference
  if(is_reference()){
    // Check if target state is in the same state where the reference is put
    if(check_same_runtime(&_lc, lc))
      _fref->push_value_to_stack();
    else // prohibit if the state address (in memory) is not the same
      push_to_stack_copy(lc);

    return;
  }

  bool _push_by_create_copy = false;

  // Is copy
  // If C function
  if(is_cfunction()){
    if(!_this_fn){
      lc->context->api_value->pushnil(lc->istate);
      return;
    }

#ifdef LUA_CODE_EXISTS
    // Check the version first
    if(verify_lua_version(cpplua_get_api_compilation_context(), lc->context)){
      // push user function as upvalue
      lc->context->api_value->pushlightuserdata(lc->istate, (void*)_this_fn);

      // END OF LUA_FUNCVAR_START_UPVALUE

      // push the upvalues first
      for(int i = 0; i < _fn_args->get_var_count(); i++)
        _fn_args->get_self_var(i)->push_to_stack(lc);

      lc->context->api_value->pushcclosure(lc->istate, _cb_entry_point, LUA_FUNCVAR_START_UPVALUE_IDX+_fn_args->get_var_count());
    }
    else
      _push_by_create_copy = true;
#else
    _push_by_create_copy = true;
#endif
  }
  // If Lua function
  else{
#ifdef LUA_CODE_EXISTS
    // Check the version first
    if(verify_lua_version(cpplua_get_api_compilation_context(), lc->context)){
      _chk_reader_data _reader_data;
        _reader_data._this = this;
        _reader_data.current_idx = 0;

      lc->context->api_state->load(lc->istate, _fn_chunk_data_reader, &_reader_data, _fn_binary_name, "b");
    }
    else
      _push_by_create_copy = true;
#else
    _push_by_create_copy = true;
#endif
  }

  if(_push_by_create_copy){
    I_variant* _fcopy = lc->context->api_varutil->create_variant_copy(this);
    _fcopy->push_to_stack(lc);
    lc->context->api_varutil->delete_variant(_fcopy);
  }
}

void function_var::push_to_stack_copy(const core* lc) const{
  // Use push_to_stack if this already has the copy
  if(!is_reference()){
    push_to_stack(lc);
    return;
  }

  function_var* _tmpvar = dynamic_cast<function_var*>(cpplua_create_var_copy(this));
  _tmpvar->as_copy();
  _tmpvar->push_to_stack(lc);
  cpplua_delete_variant(_tmpvar);
}


std::string function_var::to_string() const{
  if(is_cfunction())
    return format_str_mem(__dm, "CFunction 0x%X", (unsigned long long)_this_fn);
  else
    return format_str_mem(__dm, "LuaFunction 0x%X", (unsigned long long)_func_pointer);
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

const void* function_var::get_function_binary_chunk() const{
  return _fn_binary_data;
}

size_t function_var::get_function_binary_chunk_len() const{
  return _fn_binary_len;
}

const char* function_var::get_function_binary_chunk_name() const{
  return _fn_binary_name;
}

const void* function_var::get_lua_function_pointer() const{
  return _func_pointer;
}


bool function_var::set_function(luaapi_cfunction fn){
  if(!is_reference() || !is_cfunction())
    return false;

  _this_fn = fn;
  return true;
}


void function_var::reset_cfunction(luaapi_cfunction fn, const I_vararr* closure){
  _clear_function_data();
  _init_cfunction();

  _this_fn = fn;
  if(closure)
    _fn_args->append(closure);

  _is_reference = false;
  _is_cfunction = true;
}

void function_var::reset_luafunction(const void* chunk_buf, size_t len, const char* chunk_name){
  _clear_function_data();

  _fn_binary_len = len;
  _fn_binary_data = __dm->malloc(len, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  memcpy(_fn_binary_data, chunk_buf, len);

  size_t _name_len = chunk_name? strlen(chunk_name): 0;
  _fn_binary_name = (char*)__dm->malloc(_name_len+1, DYNAMIC_MANAGEMENT_DEBUG_DATA); // with null-termination character
  if(chunk_name)
    memcpy(_fn_binary_name, chunk_name, _name_len);
  _fn_binary_name[_name_len] = '\0';

  _is_reference = false;
  _is_cfunction = false;
}


bool function_var::is_cfunction() const{
  return _is_cfunction;
}

bool function_var::is_luafunction() const{
  return !_is_cfunction;
}

bool function_var::is_reference() const{
  return _is_reference;
}


int function_var::run_function(const core* lc, const I_vararr* args, I_vararr* results) const{
  return pstack_call_core<int>(lc, 0, 0, [](const function_var* _this, const core* lc, const I_vararr* args, I_vararr* results){
    int _previous_stack = lc->context->api_stack->gettop(lc->istate);

    _this->push_to_stack(lc);
    if(lc->context->api_value->type(lc->istate, -1) != LUA_TFUNCTION)
      return -1;
    
    for(int i = 0; i < args->get_var_count(); i++)
      args->get_var(i)->push_to_stack(lc);

    int _errcode = lc->context->api_execution->pcall(lc->istate, args->get_var_count(), LUA_MULTRET, 0);
    int _delta_stack = lc->context->api_stack->gettop(lc->istate) - _previous_stack;

    results->clear();
    for(int i = 0; i < _delta_stack; i++){
      int _idx_stack = -(_delta_stack-i);

      I_variant* _argv = lc->context->api_varutil->to_variant(lc->istate, _idx_stack);
      results->append_var(_argv);
      lc->context->api_varutil->delete_variant(_argv);
    }

    return _errcode;
  }, this, lc, args, results);
}


void function_var::as_copy(){
  if(!is_reference()) // already a copy
    return;

  push_to_stack(&_lc);
  from_state_copy(&_lc, -1);
  _lc.context->api_stack->pop(_lc.istate, 1);
}


const core* function_var::get_lua_core() const{
  return &_lc;
}




// MARK: error_var def

error_var::error_var(){}

error_var::error_var(const error_var& var){
  _copy_from_var(&var);
}

error_var::error_var(const I_error_var* var){
  _copy_from_var(var);
}

error_var::error_var(const variant* data, long long code){
  _err_data = cpplua_create_var_copy(data);
  _error_code = code;
}

error_var::error_var(const I_variant* data, long long code){
  _err_data = cpplua_create_var_copy(data);
  _error_code = code;
}

error_var::error_var(const core* lc, int stack_idx){
  from_state(lc, stack_idx);
}

error_var::~error_var(){
  _clear_object();
}


void error_var::_copy_from_var(const I_error_var* data){
  _clear_object();
  _err_data = cpplua_create_var_copy(data->get_error_data_interface());
  _error_code = data->get_error_code();
}

void error_var::_clear_object(){
  if(_err_data){
    cpplua_delete_variant(_err_data);
    _err_data = NULL;
  }
}


int error_var::get_type() const{
  return LUA_TERROR;
}


bool error_var::from_state(const core* lc, int stack_idx){
  stack_idx = lc->context->api_stack->absindex(lc->istate, stack_idx);
  _clear_object();
  _err_data = to_variant(lc, stack_idx);

  return true;
}

void error_var::push_to_stack(const core* lc) const{
  if(!_err_data){
    lc->context->api_value->pushnil(lc->istate);
    return;
  }

  _err_data->push_to_stack(lc);
}


void error_var::to_string(I_string_store* pstring) const{
  std::string _str = to_string();
  pstring->append(_str.c_str());
}

std::string error_var::to_string() const{
  if(!_err_data)
    return "nil";

  std::string _err_str = _err_data->to_string();
  return format_str_mem(__dm, "(%lld) %s", _error_code, _err_str.c_str());
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

object_var::object_var(const object_var& data){
  _copy_from_var(&data);
}

object_var::object_var(const I_object_var* data){
  _copy_from_var(data);
}

object_var::object_var(const core* lc, I_object* obj){
  from_object_reference(lc, obj);
}

object_var::object_var(const core* lc, int stack_idx){
  from_state(lc, stack_idx);
}

object_var::~object_var(){
  _clear_object();
}


void object_var::_copy_from_var(const I_object_var* data){
  _clear_object();

  _lc = *data->get_lua_core();
  _obj = data->get_object_reference();
  if(!_obj)
    return;

  _oref = __dm->new_class_dbg<value_ref>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_lc, OBJECT_REFERENCE_INTERNAL_DATA, _get_reference_key(_obj).c_str());
}

void object_var::_clear_object(){
  if(_oref){
    __dm->delete_class_dbg(_oref, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _oref = NULL;
  }

  _obj = NULL;
}


std::string object_var::_get_reference_key(void* idata){
  return format_str_mem(__dm, "0x%X", idata);
}


int object_var::_compare_with(const variant* rhs) const{
  object_var* _rhs = (object_var*)rhs;
  if(_obj == _rhs->_obj)
    return 0;

  return _obj > _rhs->_obj? 1: -1;
}


int object_var::get_type() const{
  return LUA_TCPPOBJECT;
}


bool object_var::from_state(const core* lc, int stack_idx){
  stack_idx = lc->context->api_stack->absindex(lc->istate, stack_idx);
  _clear_object();

  int _type = lc->context->api_varutil->get_special_type(lc->istate, stack_idx);
  if(_type != get_type()){
    _logger->print_error(format_str_mem(__dm, "Mismatched type when converting at stack (%d). (%s-%s)\n",
      stack_idx,
      lc->context->api_value->ttypename(lc->istate, _type),
      lc->context->api_value->ttypename(lc->istate, get_type())
    ).c_str());

    return false;
  }

  I_object* _test_obj = lc->context->api_objutil->get_object_from_table(lc->istate, stack_idx);
  if(!_test_obj)
    return false;

  _lc = *lc;
  _obj = _test_obj;
  _oref = __dm->new_class_dbg<value_ref>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_lc, OBJECT_REFERENCE_INTERNAL_DATA, _get_reference_key(_obj).c_str());
  if(!_oref->reference_initiated())
    _oref->set_value(stack_idx);

  return true;
}

bool object_var::from_object_reference(const core* lc, I_object* obj){
  _clear_object();
  
  if(!obj)
    return false;

  _lc = *lc;
  _obj = obj;
  _oref = __dm->new_class_dbg<value_ref>(DYNAMIC_MANAGEMENT_DEBUG_DATA, &_lc, OBJECT_REFERENCE_INTERNAL_DATA, _get_reference_key(_obj).c_str());
  if(!_oref->reference_initiated()){
    _lc.context->api_value->newtable(_lc.istate);
    _lc.context->api_objutil->push_object_to_table(_lc.istate, _obj, -1);
    _oref->set_value(-1);

    _lc.context->api_stack->pop(_lc.istate, 1);
  }

  return true;
}

void object_var::push_to_stack(const core* lc) const{
  // Shouldn't be pushed when different runtime state, since the object memory management already handled in another runtime state.
  if(!_oref || !check_same_runtime(lc, &_lc)){
    lc->context->api_value->pushnil(lc->istate);
    return;
  }

  _oref->push_value_to_stack();
}


#define OBJECT_VAR_STRING_VAL "Cpp-Object (Table)"

void object_var::to_string(I_string_store* pstring) const{
  pstring->append(OBJECT_VAR_STRING_VAL);
}

std::string object_var::to_string() const{
  return OBJECT_VAR_STRING_VAL;
}


I_object* object_var::get_object_reference() const{
  return _obj;
}


bool object_var::object_pushed_to_lua() const{
  return _oref != NULL && _oref->reference_initiated();
}


const core* object_var::get_lua_core() const{
  return &_lc;
}




// MARK: comparison_variant def

comparison_variant::comparison_variant(const I_variant* from){
  _init_class(from);
}

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
  cpplua_delete_variant(_this_var);
}


void comparison_variant::_init_class(const I_variant* from){
  _this_var = cpplua_create_var_copy(from);
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




// MARK: Static functions

lua::variant* cpplua_create_var_copy(const lua::I_variant* data){
  switch(data->get_type()){
    break; case string_var::get_static_lua_type():
      return __dm->new_class_dbg<string_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, dynamic_cast<const I_string_var*>(data));

    break; case number_var::get_static_lua_type():
      return __dm->new_class_dbg<number_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, dynamic_cast<const I_number_var*>(data));

    break; case bool_var::get_static_lua_type():
      return __dm->new_class_dbg<bool_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, dynamic_cast<const I_bool_var*>(data));

    break; case table_var::get_static_lua_type():
      return __dm->new_class_dbg<table_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, dynamic_cast<const I_table_var*>(data));

    break; case lightuser_var::get_static_lua_type():
      return __dm->new_class_dbg<lightuser_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, dynamic_cast<const I_lightuser_var*>(data));

    break; case error_var::get_static_lua_type():
      return __dm->new_class_dbg<error_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, dynamic_cast<const I_error_var*>(data));

    break; case object_var::get_static_lua_type():
      return __dm->new_class_dbg<object_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, dynamic_cast<const I_object_var*>(data));

    break; case function_var::get_static_lua_type():
      return __dm->new_class_dbg<function_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA, dynamic_cast<const I_function_var*>(data));

    break; default:
      return __dm->new_class_dbg<nil_var>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
  }
}


// MARK: DLL functions

DLLEXPORT void CPPLUA_VARIANT_SET_DEFAULT_LOGGER(I_logger* _logger){
  set_default_logger(_logger);
}

DLLEXPORT void CPPLUA_DELETE_VARIANT(const lua::I_variant* data){
  __dm->delete_class_dbg(dynamic_cast<const variant*>(data), DYNAMIC_MANAGEMENT_DEBUG_DATA);
}