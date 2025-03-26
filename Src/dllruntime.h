#ifndef DLLRUNTIME_HEADER
#define DLLRUNTIME_HEADER

#if (_WIN64) || (_WIN32)

#else

enum def_dllevent_mode{
  def_dllevent_starting,
  def_dllevent_finishing,
  def_dllevent_thread_start,
  def_dllevent_thread_stop
};

#endif // #if (_WIN64) || (_WIN32)

#endif