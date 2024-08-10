#ifndef I_DEBUG_USER_HEADER
#define I_DEBUG_USER_HEADER

#include "I_logger.h"

class I_debug_user{
  public:
    virtual void set_logger(I_logger* logger){};
};

#endif