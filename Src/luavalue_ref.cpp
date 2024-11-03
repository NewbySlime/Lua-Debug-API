#include "luaapi_internal.h"
#include "luaapi_stack.h"
#include "luaapi_value.h"
#include "luainternal_storage.h"
#include "luautility.h"
#include "luavalue_ref.h"

#define VALUE_REF_METADATA_REF_COUNT "ref_count"
#define VALUE_REF_METADATA_ACTUAL_VALUE "_this"


using namespace lua;
using namespace lua::api;
using namespace lua::internal;
using namespace lua::utility;



value_ref::value_ref(const lua::api::core* lua_core, const char* reference_key, const char* value_key){
  _lc = *lua_core;
  _ref_key = reference_key;
  _value_key = value_key;

  _initiate_class();
}

value_ref::value_ref(const value_ref& ref){
  _lc = ref._lc;
  _ref_key = ref._ref_key;
  _value_key = ref._value_key;

  _initiate_class();
}

value_ref::~value_ref(){
  // reference initiated at the time of creation / value set
  int _reference_count = _get_reference_count();
  if(_reference_initiated && _reference_count > 0){
    if(_reference_count == 1)
      _delete_metadata();
    else
      _decrement_reference();
  }
}


void value_ref::_require_internal_data(){
  // internal storage s+1
  _lc.context->api_internal->require_internal_storage(_lc.istate);

  _lc.context->api_value->pushstring(_lc.istate, _ref_key.c_str());
  if(_lc.context->api_value->gettable(_lc.istate, -2) != LUA_TTABLE){
    _lc.context->api_stack->pop(_lc.istate, 1); // pop the last value first

    _lc.context->api_value->newtable(_lc.istate); // don't pop, it will be the result

    // set internal data in the internal storage
    _lc.context->api_value->pushstring(_lc.istate, _ref_key.c_str());
    _lc.context->api_stack->pushvalue(_lc.istate, -2); // copy the new table
    _lc.context->api_value->settable(_lc.istate, -4);
  }

  _lc.context->api_stack->remove(_lc.istate, -2); // remove the internal storage
}


void value_ref::_create_metadata(){
  // internal data s+1
  _require_internal_data();

  // set new metadata table to the internal data (later)
  _lc.context->api_value->pushstring(_lc.istate, _value_key.c_str());
  _lc.context->api_value->newtable(_lc.istate);
  
  // set value
  _lc.context->api_value->pushstring(_lc.istate, VALUE_REF_METADATA_ACTUAL_VALUE);
  _lc.context->api_stack->pushvalue(_lc.istate, -5);
  _lc.context->api_value->settable(_lc.istate, -3);

  // set reference count
  _lc.context->api_value->pushstring(_lc.istate, VALUE_REF_METADATA_REF_COUNT);
  _lc.context->api_value->pushinteger(_lc.istate, 1);
  _lc.context->api_value->settable(_lc.istate, -3);

  _lc.context->api_value->settable(_lc.istate, -3);

  _lc.context->api_stack->pop(_lc.istate, 1); // pop internal data
}

void value_ref::_delete_metadata(){
  // internal data s+1
  _require_internal_data();

  // set the reference metadata to nil
  _lc.context->api_value->pushstring(_lc.istate, _value_key.c_str());
  _lc.context->api_value->pushnil(_lc.istate);
  _lc.context->api_value->settable(_lc.istate, -3);

  _lc.context->api_stack->pop(_lc.istate, 1); // pop internal data
}

void value_ref::_push_metadata(){
  // internal data s+1
  _require_internal_data();

  // get metadata
  _lc.context->api_value->pushstring(_lc.istate, _value_key.c_str());
  _lc.context->api_value->gettable(_lc.istate, -2);

  _lc.context->api_stack->remove(_lc.istate, -2); // remove internal data
}


#define _FPART_REFCOUNT_MODIFY_BY_ONE(operand) \
  /* metadata table s+1 */ \
  _push_metadata(); \
   \
  /* get reference count */ \
  _lc.context->api_value->pushstring(_lc.istate, VALUE_REF_METADATA_REF_COUNT); \
  _lc.context->api_stack->pushvalue(_lc.istate, -1); /* push key for later use (set reference count) */ \
  _lc.context->api_value->gettable(_lc.istate, -3); \
   \
  /* increment the count */ \
  _lc.context->api_value->pushinteger(_lc.istate, 1); \
  _lc.context->api_value->arith(_lc.istate, operand); \
   \
  /* set reference count */ \
  _lc.context->api_value->settable(_lc.istate, -3); \
   \
  _lc.context->api_stack->pop(_lc.istate, 1); /* pop metadata table */

void value_ref::_increment_reference(){
  _FPART_REFCOUNT_MODIFY_BY_ONE(LUA_OPADD)
}

void value_ref::_decrement_reference(){
  _FPART_REFCOUNT_MODIFY_BY_ONE(LUA_OPSUB)
}


int value_ref::_get_reference_count(){
  return pstack_call_core<int>(&_lc, 0, 0, [](value_ref* _this){
    core& _lc = _this->_lc;

    // metadata table s+1
    _this->_push_metadata();
    if(_lc.context->api_value->type(_lc.istate, -1) != LUA_TTABLE)
      return -1;

    // get reference count s+1
    _lc.context->api_value->pushstring(_lc.istate, VALUE_REF_METADATA_REF_COUNT);
    _lc.context->api_value->gettable(_lc.istate, -2);

    int _result = _lc.context->api_value->tointeger(_lc.istate, -1);
    return _result; // stack will be stabilized by pstack_call
  }, this);
}


void value_ref::_initiate_class(){
  _reference_initiated = false;
  update_reference();
}


bool value_ref::has_reference(){
  _push_metadata();
  bool _result = _lc.context->api_value->type(_lc.istate, -1) == LUA_TTABLE;
  _lc.context->api_stack->pop(_lc.istate, 1); // pop the metadata

  return _result;
}


void value_ref::update_reference(){
  if(has_reference() && !_reference_initiated){
    _increment_reference();
    _reference_initiated = true;
  }
}

bool value_ref::reference_initiated(){
  return _reference_initiated;
}


void value_ref::push_value_to_stack(){
  if(has_reference()){
    update_reference();

    _push_metadata();

    _lc.context->api_value->pushstring(_lc.istate, VALUE_REF_METADATA_ACTUAL_VALUE);
    _lc.context->api_value->gettable(_lc.istate, -2);

    _lc.context->api_stack->remove(_lc.istate, -2); // remove metadata
  }
  else
    _lc.context->api_value->pushnil(_lc.istate);
}

void value_ref::set_value(int value_sidx){
  value_sidx = _lc.context->api_stack->convtop2bottom(_lc.istate, value_sidx); // in case of idx is dependent of top stack

  if(has_reference()){
    update_reference();

    _push_metadata();

    _lc.context->api_value->pushstring(_lc.istate, VALUE_REF_METADATA_ACTUAL_VALUE);
    _lc.context->api_stack->pushvalue(_lc.istate, value_sidx);
    _lc.context->api_value->settable(_lc.istate, -3);

    _lc.context->api_stack->pop(_lc.istate, 1); // pop the metadata
  }
  else{ // create the metadata if reference has not been created
    _lc.context->api_stack->pushvalue(_lc.istate, value_sidx); // copy the value first
    _create_metadata();
    _lc.context->api_stack->pop(_lc.istate, 1);

    _reference_initiated = true;
  }
}


const core* value_ref::get_lua_core(){
  return &_lc;
}