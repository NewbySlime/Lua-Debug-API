#ifdef LUA_CODE_EXISTS

#include "luainternal_storage.h"
#include "luamemory_util.h"
#include "luastate_metadata.h"
#include "luautility.h"

#include "map"
#include "set"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif


using namespace lua;
using namespace lua::internal;
using namespace lua::memory;
using namespace lua::object;
using namespace lua::state;
using namespace lua::utility;
using namespace ::memory;

#define METADATA_OBJECT_KEY "__cpplua_state_metadata"


static I_dynamic_management* __dm = get_memory_manager();


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
  I_metadata* _this_metadata = get_metadata(state);
  if(_this_metadata)
    return _this_metadata;

  _this_metadata = __dm->new_class_dbg<metadata>(DYNAMIC_MANAGEMENT_DEBUG_DATA, state);
  void** _extraspace = (void**)lua_getextraspace(state);
  *_extraspace = _this_metadata;
  return _this_metadata;
}

void lua::state::deinitiate_metadata(lua_State* state){
  I_metadata* _this_metadata = get_metadata(state);
  if(!_this_metadata)
    return;

  __dm->delete_class_dbg(_this_metadata, DYNAMIC_MANAGEMENT_DEBUG_DATA);
  void** _extraspace = (void**)lua_getextraspace(state);
  *_extraspace = NULL;
}

I_metadata* lua::state::get_metadata(lua_State* state){
  void** _extraspace = (void**)lua_getextraspace(state);
  return (metadata*)*_extraspace;
}


#endif // LUA_CODE_EXISTS