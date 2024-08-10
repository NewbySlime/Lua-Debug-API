#ifndef STDLOGGER_HEADER
#define STDLOGGER_HEADER

#include "I_logger.h"


class stdlogger: public I_logger{
  public:
    void print(std::string message) const override;
    void print_warning(std::string message) const override;
    void print_error(std::string message) const override;

    void print(const char* message) const override;
    void print_warning(const char* message) const override;
    void print_error(const char* message) const override;
};


I_logger* get_stdlogger();


#endif