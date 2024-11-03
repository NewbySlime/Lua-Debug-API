#include "error_util.h"


using namespace error::util;


#if (_WIN64) || (_WIN32)
std::string error::util::get_windows_error_message(DWORD error_code){
  LPSTR _buffer;
  FormatMessageA(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    NULL,
    error_code,
    0,
    (LPSTR)&_buffer,
    0,
    NULL
  );

  std::string _result = _buffer;
  LocalFree(_buffer);
  return _result;
}
#endif