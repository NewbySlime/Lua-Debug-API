#include "dllutil.h"
#include "error_util.h"
#include "memdynamic_management.h"

#if (_WIN64) || (_WIN32)
#include "Windows.h"
#endif

#include "map"
#include "memory"

using namespace dynamic_library::util;
using namespace error::util;
using namespace memory;


// NOTE: don't overload malloc, free or other C std function for memory management as it is used for the default option. 

#ifdef USING_MEMDYNAMIC_MANAGEMENT

static void* _default_malloc(size_t size, void* data);
static void _default_free(void* ptr, void* data);
static void* _default_realloc(void* ptr, size_t size, void* data);
static memory_management_config _config = memory_management_config(_default_malloc, _default_free, _default_realloc, NULL, NULL);
// static nested_class_record* _usage_record_object = NULL; // only used in new and delete operators
static memory_debug_data _debug_data;

thread_local nested_class_record* _nested_record_tls = NULL;

static void _code_initiate();


static void _reset_config(){
  _config = memory_management_config(_default_malloc, _default_free, _default_realloc, NULL, NULL);
}


static void _code_initiate(){
  if(!_config.f_alloc)
    _reset_config();
}


static void _acquire_memlock(bool acquire){
  if(_config.f_sync)
    _config.f_sync(acquire, _config.f_data);
}


static void _add_nested_record(nested_class_record* record){
  record->tail = _nested_record_tls;
  _nested_record_tls = record;
}

static void _remove_nested_record(){
  if(!_nested_record_tls)
    return;

  _nested_record_tls = _nested_record_tls->tail;
}


// NOTE: this uses _debug_data as the sent data.
static void _signal_debug_data(){
#ifdef DEBUG_BUILD
  if(_config.f_dbg && _debug_data.function_name)
    _config.f_dbg(&_debug_data, _config.f_data);
#endif // DEBUG_BUILD
}

// NOTE: this uses _debug_data as the sent data. Only reset when _debug_data is not NULL.
static void _signal_debug_data_reset(){
#ifdef DEBUG_BUILD
  if(_config.f_dbg && _debug_data.function_name)
    _config.f_dbg(NULL, _config.f_data);
#endif // DEBUG_BUILD
}


static void _set_debug_data(const ::memory::memory_debug_data* debug_data){
  _debug_data = ::memory::memory_debug_data();
  if(debug_data)
    _debug_data = *debug_data;
}


#define __CALL_MEM_INIT(debug_data) \
  _acquire_memlock(true); \
  _set_debug_data(debug_data); \
  _signal_debug_data();

#define __CALL_MEM_DEINIT() \
  _signal_debug_data_reset(); \
  _set_debug_data(NULL); \
  _acquire_memlock(false);


static void* _malloc(size_t size, const memory_debug_data* dbg_data){
  _code_initiate();
  __CALL_MEM_INIT(dbg_data)
  void* _result = _config.f_alloc(size, _config.f_data);
  __CALL_MEM_DEINIT()
  return _result;
}

static void _free(void* obj, const memory_debug_data* dbg_data){
  _code_initiate();
  __CALL_MEM_INIT(dbg_data)
  _config.f_dealloc(obj, _config.f_data);
  __CALL_MEM_DEINIT()
}

static void* _realloc(void* target_obj, size_t size, const memory_debug_data* dbg_data){
  _code_initiate();
  __CALL_MEM_INIT(dbg_data)
  void* _result = _config.f_realloc(target_obj, size, _config.f_data);
  __CALL_MEM_DEINIT()

  return _result;
}


void* operator new(size_t size){
  return _malloc(size, _nested_record_tls? _nested_record_tls->debug_data: NULL);
}

void operator delete(void* ptr){
  _free(ptr, _nested_record_tls? _nested_record_tls->debug_data: NULL);
}


static void* _default_malloc(size_t size, void* data){
  return std::malloc(size);
}

static void _default_free(void* ptr, void* data){
  std::free(ptr);
}

static void* _default_realloc(void* ptr, size_t size, void* data){
  return std::realloc(ptr, size);
}


class _dynamic_management: public I_dynamic_management{
  protected:
    void _add_nested_record(nested_class_record* record) const override{
      ::_add_nested_record(record);
    }

    void _remove_nested_record() const override{
      ::_remove_nested_record();
    }


  public:
    void* malloc(size_t size, const memory_debug_data& dbg_data) const override{
      return _malloc(size, &dbg_data);
    }

    void* malloc(size_t size, const memory_debug_data* dbg_data) const override{
      return _malloc(size, dbg_data);
    }

    void* malloc(size_t size) const override{
      return _malloc(size, NULL);
    }

    void free(void* obj, const memory_debug_data& dbg_data) const override{
      _free(obj, &dbg_data);
    }

    void free(void* obj, const memory_debug_data* dbg_data) const override{
      _free(obj, dbg_data);
    }

    void free(void* obj) const override{
      _free(obj, NULL);
    }


    void* realloc(void* target_obj, size_t size, const memory_debug_data& dbg_data) const override{
      return _realloc(target_obj, size, &dbg_data);
    }

    void* realloc(void* target_obj, size_t size, const memory_debug_data* dbg_data) const override{
      return _realloc(target_obj, size, dbg_data);
    }

    void* realloc(void* target_obj, size_t size) const override{
      return _realloc(target_obj, size, NULL);
    }


    void set_config(const memory_management_config* config) const override{
      _config = *config;
    }

    void reset_config() const override{
      _reset_config();
    }

    const memory_management_config* get_config() const override{
      return &_config;
    }
};


static _dynamic_management _dm;

DLLEXPORT I_dynamic_management* MEMDYNAMIC_MANAGEMENT_GET_STATIC_INSTANCE(){
  return &_dm;
}

#endif // USING_MEMDYNAMIC_MANAGEMENT