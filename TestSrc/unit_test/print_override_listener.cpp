#include "../../src/string_util.h"
#include "print_override_listener.h"

using namespace lua::global;


print_override_listener::print_override_listener(I_print_override* po_obj, I_logger* logger, I_logger* dbg_logger){
  _po_obj = po_obj;
  _logger = logger;
  _dbg_logger = dbg_logger;

  _output_event = CreateEventA(NULL, true, false, NULL);
  _po_obj->register_event_read(_output_event);

  _this_mutex = CreateMutex(NULL, false, NULL);
}

print_override_listener::~print_override_listener(){
  stop_listening();
  
  _po_obj->remove_event_read(_output_event);
  CloseHandle(_output_event);
  CloseHandle(_this_mutex);
}


DWORD print_override_listener::_listen_to_print_override(){
  size_t _peek_buffer_len = 5;
  char* _peek_buffer = (char*)malloc(_peek_buffer_len+1);

  while(true){
WaitForSingleObject(_this_mutex, INFINITE); // lock _this_mutex
    bool _keep_loop = _keep_listening;
ReleaseMutex(_this_mutex); // release _this_mutex
    if(!_keep_loop)
      break;

    WaitForSingleObject(_output_event, INFINITE);
WaitForSingleObject(_this_mutex, INFINITE); // lock _this_mutex
    if(_po_obj->available_bytes() > 0){
      _po_obj->peek_n(_peek_buffer, _peek_buffer_len);
      string_store _str; _po_obj->read_all(&_str);
      for(int i = 0; i < _peek_buffer_len && i < _str.data.length(); i++){
        if(_peek_buffer[i] != _str.data[i]){
          _dbg_logger->print_error(format_str("Peek data and read data does not match. Data: %s\n", _str.data.c_str()).c_str());
          break;
        }
      }

      _logger->print(_str.data.c_str());
    }
ReleaseMutex(_this_mutex); // release _this_mutex
  }

  free(_peek_buffer);
  return S_OK;
}

DWORD WINAPI print_override_listener::_listen_to_print_override_trigger(LPVOID data){
  return ((print_override_listener*)data)->_listen_to_print_override();
}


bool print_override_listener::currently_listening(){
  return _thread_handle != NULL;
}


bool print_override_listener::start_listening(){
  if(currently_listening())
    return false;

  _keep_listening = true;
  _thread_handle = CreateThread(NULL, 0, _listen_to_print_override_trigger, this, 0, NULL);
  return true;
}

bool print_override_listener::stop_listening(){
  if(!currently_listening())
    return false;

WaitForSingleObject(_this_mutex, INFINITE); // lock _this_mutex
  _keep_listening = false;
  SetEvent(_output_event); // skip waiting for output signal
ReleaseMutex(_this_mutex); // release _this_mutex

  WaitForSingleObject(_thread_handle, INFINITE); // "join" the thread
  CloseHandle(_thread_handle);
  _thread_handle = NULL;

  return true;
}