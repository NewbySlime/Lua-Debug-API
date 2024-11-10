#ifdef LUA_CODE_EXISTS

#include "dllutil.h"
#include "luainternal_storage.h"
#include "luamemory_util.h"
#include "luastate_metadata.h"
#include "luautility.h"

#include "../LuaSrc/lstate.h"

#include "map"
#include "set"  

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif


using namespace dynamic_library::util;
using namespace lua;
using namespace lua::internal;
using namespace lua::memory;
using namespace lua::object;
using namespace lua::state;
using namespace lua::utility;
using namespace ::memory;


typedef std::map<void*, I_metadata*> metadata_mapping;

static I_dynamic_management* __dm = get_memory_manager();

static metadata_mapping* _metadata_data = NULL;

#if (_WIN64) || (_WIN32)
static CRITICAL_SECTION* _code_mutex = NULL;
#endif

static void _lock_code();
static void _unlock_code();

static void _code_initiate();
static void _code_deinitiate();
static destructor_helper _dh(_code_deinitiate);


static void _code_initiate(){
  if(!_metadata_data)
    _metadata_data = __dm->new_class_dbg<metadata_mapping>(DYNAMIC_MANAGEMENT_DEBUG_DATA);
  
#if (_WIN64) || (_WIN32)
  if(!_code_mutex){
    _code_mutex = (CRITICAL_SECTION*)__dm->malloc(sizeof(CRITICAL_SECTION), DYNAMIC_MANAGEMENT_DEBUG_DATA);
    InitializeCriticalSection(_code_mutex);
  }
#endif
}

static void _code_deinitiate(){
  if(_metadata_data){
    _lock_code();
    for(auto mpair: *_metadata_data)
      __dm->delete_class_dbg(mpair.second, DYNAMIC_MANAGEMENT_DEBUG_DATA);

    __dm->delete_class_dbg(_metadata_data, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _metadata_data = NULL;
    _unlock_code();
  }

#if (_WIN64) || (_WIN32)
  if(_code_mutex){
    DeleteCriticalSection(_code_mutex);
    __dm->free(_code_mutex, DYNAMIC_MANAGEMENT_DEBUG_DATA);
    _code_mutex = NULL;
  }
#endif
}


static void _lock_code(){
#if (_WIN64) || (_WIN32)
  EnterCriticalSection(_code_mutex);
#endif
}

static void _unlock_code(){
#if (_WIN64) || (_WIN32)
  LeaveCriticalSection(_code_mutex);
#endif
}


static void _def_destructor_metadata(I_object* obj){
  __dm->delete_class_dbg(obj, DYNAMIC_MANAGEMENT_DEBUG_DATA);
}

static fdata _metadata_functions[] = {
  fdata(NULL, NULL)
};

class metadata: public I_metadata, public lua::object::function_store, virtual public I_object{
  private:
    std::map<std::string, lua::I_object*> _obj_data;

#if (_WIN64) || (_WIN32)
    CRITICAL_SECTION _mutex;
#endif

  public:
    metadata(lua_State* state): function_store(_def_destructor_metadata){
      set_function_data(_metadata_functions);

#if (_WIN64) || (_WIN32)
      InitializeCriticalSection(&_mutex);
#endif
    }
    
    ~metadata(){
      std::set<I_object*> _delete_obj;
      for(auto _pair: _obj_data)
        _delete_obj.insert(_pair.second);

      _obj_data.clear();
      for(auto _iter: _delete_obj)
        _iter->get_this_destructor()(_iter);

#if (_WIN64) || (_WIN32)
      DeleteCriticalSection(&_mutex);
#endif
    }


    lua::I_object* get_object(const char* key) const override{
      auto _iter = _obj_data.find(key);
      if(_iter == _obj_data.end())
        return NULL;

      return _iter->second;
    }

    void set_object(const char* key, lua::I_object* obj) override{
      _obj_data[key] = obj;
    }

    void remove_object(const char* key) override{
      auto _iter = _obj_data.find(key);
      if(_iter == _obj_data.end())
        return;

      _obj_data.erase(_iter);
    }

    bool has_object(const char* key) const override{
      auto _iter = _obj_data.find(key);
      return _iter != _obj_data.end();
    }


    void lock_metadata() override{
#if (_WIN64) || (_WIN32)
      EnterCriticalSection(&_mutex);
#endif
    }

    void unlock_metadata() override{
#if (_WIN64) || (_WIN32)
      LeaveCriticalSection(&_mutex);
#endif
    }


    I_debuggable_object* as_debug_object() override{
      return NULL;
    }

    void on_object_added(const lua::api::core* lua_core) override{
      
    }
};


I_metadata* lua::state::initiate_metadata(lua_State* state){
  _code_initiate();
  _lock_code();

  I_metadata* _this_metadata = NULL;

{ // enclosure for using gotos
  auto _iter = _metadata_data->find(state->l_G->mainthread);
  if(_iter != _metadata_data->end()){
    _this_metadata = _iter->second;
    goto skip_to_return;
  }

  _this_metadata = __dm->new_class_dbg<metadata>(DYNAMIC_MANAGEMENT_DEBUG_DATA, state);
  _metadata_data->operator[](state->l_G->mainthread) = _this_metadata;
} // enclosure closing

  skip_to_return:{}
  _unlock_code();
  return _this_metadata;
}

void lua::state::deinitiate_metadata(lua_State* state, bool is_mainthread){
  if(!_metadata_data)
    return;

  _lock_code();

{ // enclosure for using gotos
  if(!is_mainthread)
    state = state->l_G->mainthread;

  auto _iter = _metadata_data->find(state);
  if(_iter == _metadata_data->end())
    goto skip_to_return;

  __dm->delete_class_dbg(_iter->second, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  _metadata_data->erase(_iter);
} // enclosure closing

  skip_to_return:{}
  _unlock_code();
}

I_metadata* lua::state::get_metadata(lua_State* state){
  _code_initiate();
  _lock_code();

  I_metadata* _result = NULL;

{ // enclosure for using gotos
  auto _iter = _metadata_data->find(state->l_G->mainthread);
  if(_iter == _metadata_data->end())
    goto skip_to_return;

  _result = _iter->second;
} // enclosure closing
  
  skip_to_return:{}
  _unlock_code();
  return _result;
}


#endif // LUA_CODE_EXISTS