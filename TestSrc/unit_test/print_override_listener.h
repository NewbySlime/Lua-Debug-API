#ifndef PRINT_OVERRIDE_LISTENER_HEADER
#define PRINT_OVERRIDE_LISTENER_HEADER

#include "../../src/luaglobal_print_override.h"
#include "Windows.h"


class print_override_listener{
  private:
    lua::global::I_print_override* _po_obj;
    I_logger* _logger;
    I_logger* _dbg_logger;

    HANDLE _thread_handle = NULL;
    HANDLE _this_mutex;
    HANDLE _output_event;

    bool _keep_listening = true;

    DWORD _listen_to_print_override();
    static DWORD WINAPI _listen_to_print_override_trigger(LPVOID data);

  public:
    print_override_listener(lua::global::I_print_override* po_obj, I_logger* logger, I_logger* dbg_logger);
    ~print_override_listener();

    bool currently_listening();

    bool start_listening();
    bool stop_listening();
};

#endif