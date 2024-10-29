#include "../../src/string_util.h"
#include "error_util.h"
#include "exception"
#include "file_logger.h"
#include "system_error"

using namespace error::util;


file_logger::file_logger(const std::string& file_path, bool append_file){
  _file_handle = CreateFile(
    file_path.c_str(),
    GENERIC_WRITE,
    0,
    NULL,
    append_file? OPEN_ALWAYS: CREATE_ALWAYS,
    0,
    NULL
  );

  if(_file_handle == INVALID_HANDLE_VALUE){
    std::string _errmsg = format_str("Cannot create file_logger: %s\n", get_windows_error_message(GetLastError()).c_str());
    printf(_errmsg.c_str());
    throw std::runtime_error(_errmsg);
  }

  if(append_file)
    SetFilePointer(_file_handle, 0, NULL, FILE_END);
}

file_logger::~file_logger(){
  if(is_closed())
    return;

  close();
}


void file_logger::print(const char* message) const{
  if(is_closed())
    return;

  size_t _message_len = strlen(message);
  bool _success = WriteFile(_file_handle, message, _message_len, NULL, NULL);
  if(!_success){
    std::string _errmsg = format_str("Cannot print to file: %s\n", get_windows_error_message(GetLastError()).c_str());
    printf(_errmsg.c_str());
    throw std::runtime_error(_errmsg);
  }
}

void file_logger::print_warning(const char* message) const{
  print(message);
}

void file_logger::print_error(const char* message) const{
  print(message);
}


void file_logger::flush(){
  if(is_closed())
    return;

  FlushFileBuffers(_file_handle);
}

void file_logger::close(){
  if(is_closed())
    return;

  flush();
  CloseHandle(_file_handle);
  _file_handle = INVALID_HANDLE_VALUE;
}


bool file_logger::is_closed() const{
  return _file_handle == INVALID_HANDLE_VALUE;
}