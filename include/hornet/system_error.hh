#ifndef HORNET_SYSTEM_ERROR_HH
#define HORNET_SYSTEM_ERROR_HH

#include <system_error>

inline void throw_system_error(std::string what)
{
    throw std::system_error(errno, std::system_category(), what);
}

#endif
