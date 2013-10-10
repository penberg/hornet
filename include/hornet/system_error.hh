#ifndef HORNET_SYSTEM_ERROR_HH
#define HORNET_SYSTEM_ERROR_HH

#include <system_error>

#define THROW_ERRNO(what)							\
	do {									\
		throw std::system_error(errno, std::system_category(), what);	\
	} while (0)

#endif
