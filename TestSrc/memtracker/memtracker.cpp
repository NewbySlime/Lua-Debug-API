#include "../../src/string_util.h"
#include "exception"
#include "memtracker.h"
#include "stdio.h"

using namespace memory;


// TODO don't use this, use another memory tracker analysis tool, try use valgrind once linux can be used.
// Dr Memory cannot be used for now in Windows 10 19045
// https://github.com/DynamoRIO/drmemory/issues/2502
// MARK: memtracker definition

static void* _memtracker_allocator(size_t size, void* f_data){
  return ((memtracker*)f_data)->allocate_memory(size);
}

static void _memtracker_deallocator(void* address, void* f_data){
  ((memtracker*)f_data)->deallocate_memory(address);
}

static void* _memtracker_reallocator(void* address, size_t new_size, void* f_data){
  return ((memtracker*)f_data)->reallocate_memory(address, new_size);
}

static void _memtracker_lock(bool lock, void* f_data){
  ((memtracker*)f_data)->lock_memory(lock);
}

static void _memtracker_set_memory_debug_data(const memory_debug_data* data, void* f_data){
  ((memtracker*)f_data)->set_memory_debug_data(data);
}


memtracker::memtracker(): _management_config(_memtracker_allocator, _memtracker_deallocator, _memtracker_reallocator, _memtracker_lock, _memtracker_set_memory_debug_data, this){
  set_memory_debug_data(NULL);

  _this_obj_creation_time = std::chrono::steady_clock::now();
  _maximal_reached_time = _this_obj_creation_time;

  InitializeCriticalSection(&_mem_mutex);
}

memtracker::~memtracker(){
  for(auto _pair: _allocated_memory)
    delete _pair.second;

  _allocated_memory.clear();
  DeleteCriticalSection(&_mem_mutex);
}


std::string memtracker::_get_relative_time_str(const std::chrono::time_point<std::chrono::steady_clock>& time) const{
  return format_str("+%dms", std::chrono::duration_cast<std::chrono::milliseconds>(time-_this_obj_creation_time).count());
}


void memtracker::_add_usage(size_t mem_size){
  _current_usage += mem_size;
  if(_current_usage > _maximum_usage)
    _maximum_usage = _current_usage;
}

void memtracker::_sub_usage(size_t mem_size){
  _current_usage -= mem_size;
}


memtracker::_memory_data* memtracker::_create_new_metadata(void* address, size_t size){
  _memory_data* _metadata = new _memory_data();
    _metadata->address = address;
    _metadata->size = size;
    _metadata->time_created = std::chrono::steady_clock::now();
    _metadata->function_created = _current_dbg_data.function_name;
    _metadata->line_created = _current_dbg_data.code_line;

  return _metadata;
}


void memtracker::check_memory_usage(I_logger* logger) const{
  logger->print("======== MEM USAGE CHECKING ========\n");
  logger->print("(Included timings are relative to the creation of memtracker)\n\n");

  logger->print(format_str("Current allocated space: %d byte(s)\n", _current_usage).c_str());
  logger->print(format_str("Maximal allocated space: %d byte(s), at %s\n", _maximum_usage, _get_relative_time_str(_maximal_reached_time).c_str()).c_str());
  logger->print("\n");
  logger->print("-------- Currently Active Address (Can be interpreted as mem leaks) --------\n\n");

  size_t _num_idx = 1;
  for(auto _pair: _allocated_memory){
    _memory_data* _m = _pair.second;
    logger->print(format_str("%d. 0x%X | size %d | created at %s::%d %s\n", _num_idx, _m->address, _m->size, _m->function_created.c_str(), _m->line_created, _get_relative_time_str(_m->time_created).c_str()).c_str());

    _num_idx++;
  }
  logger->print("\n");

  logger->print("======== END OF MEM USAGE CHECKING ========\n\n");
}


void* memtracker::allocate_memory(size_t size){
  void* _new_mem = malloc(size);
  //printf("MEMTRACKER alloc 0x%X at %s::%d\n", (unsigned int)_new_mem, _current_dbg_data? _current_dbg_data->function_name: "???", _current_dbg_data? _current_dbg_data->code_line: -1);

  _memory_data* _metadata = _create_new_metadata(_new_mem, size);
  _add_usage(size);
  _allocated_memory[_new_mem] = _metadata;

  return _new_mem;
}

void memtracker::deallocate_memory(void* address){
  //printf("MEMTRACKER dealloc 0x%X at %s::%d\n", (unsigned int)address, _current_dbg_data? _current_dbg_data->function_name: "???", _current_dbg_data? _current_dbg_data->code_line: -1);

  auto _iter = _allocated_memory.find(address);
  if(_iter == _allocated_memory.end()){
    printf("ERROR: Deallocating an address (0x%X) that is not created in the same memtracker.\n", (unsigned int)address);
    throw std::runtime_error("Bad Memory");
  }

  _sub_usage(_iter->second->size);
  delete _iter->second;
  _allocated_memory.erase(_iter);

  free(address);
}

void* memtracker::reallocate_memory(void* address, size_t new_size){
  auto _iter = _allocated_memory.find(address);
  if(_iter == _allocated_memory.end()){
    printf("ERROR: Reallocating an address (0x%X) that is not created in the same memtracker.\n", (unsigned int)address);
    throw std::runtime_error("Bad Memory");
  }

  _sub_usage(_iter->second->size);
  delete _iter->second;
  _allocated_memory.erase(_iter);

  void* _new_memory = realloc(address, new_size);

  _memory_data* _metadata = _create_new_metadata(_new_memory, new_size);
  _add_usage(new_size);
  _allocated_memory[_new_memory] = _metadata;

  return _new_memory;
}

void memtracker::lock_memory(bool acquire){
  if(acquire)
    EnterCriticalSection(&_mem_mutex);
  else
    LeaveCriticalSection(&_mem_mutex);
}

void memtracker::set_memory_debug_data(const memory_debug_data* data){
  if(!data){
    _current_dbg_data.function_name = "???";
    _current_dbg_data.code_line = 0;
    return;
  }

  _current_dbg_data = *data;
  _current_dbg_data.function_name = data->function_name? data->function_name: "???";
}


size_t memtracker::get_maximal_usage() const{
  return _maximum_usage;
}

size_t memtracker::get_current_usage() const{
  return _current_usage;
}


const memory_management_config* memtracker::get_configuration() const{
  return &_management_config;
}



// MARK: DLL Functions

DLLEXPORT I_memtracker* MEMTRACKER_CONSTRUCT(){
  return new memtracker();
}

DLLEXPORT void MEMTRACKER_DECONSTRUCT(I_memtracker* tracker){
  delete tracker;
}