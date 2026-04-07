#ifndef ERROR_HPP
#define ERROR_HPP

#include <string>

enum ErrFunc {
    EF_NONE = 0,
    EF_SOCKET = 1,
    EF_SETSOCKOPT = 2,
    EF_BIND = 3,
    EF_LISTEN = 4,
    EF_ACCEPT = 5,
    EF_POLL = 6,
    EF_RECV = 7,
    EF_SEND = 8,
    EF_FCNTL = 9,
};

std::string err_word( const int err_no, const ErrFunc func );

#endif