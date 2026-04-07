#include "Error.hpp"

#include <cstring>   // std::strerror
#include <cerrno>    // errno, E*
#include <string>

// ------------------------------------------------------------
// errno symbol mappers (per API)
// ------------------------------------------------------------
static std::string errno_socket( int e )
{
    switch (e) {
        case EAFNOSUPPORT:    return "EAFNOSUPPORT";
        case EPROTONOSUPPORT: return "EPROTONOSUPPORT";
        case EMFILE:          return "EMFILE";
        case ENFILE:          return "ENFILE";
        case EACCES:          return "EACCES";
#ifdef ENOBUFS
        case ENOBUFS:         return "ENOBUFS";
#endif
        case ENOMEM:          return "ENOMEM";
        default:              return "UNKNOWN_ERR";
    }
}

static std::string errno_setsockopt( int e )
{
    switch (e) {
        case EBADF:         return "EBADF";
#ifdef ENOTSOCK
        case ENOTSOCK:      return "ENOTSOCK";
#endif
#ifdef ENOPROTOOPT
        case ENOPROTOOPT:   return "ENOPROTOOPT";
#endif
        case EINVAL:        return "EINVAL";
        case EFAULT:        return "EFAULT";
        case ENOMEM:        return "ENOMEM";
        default:            return "UNKNOWN_ERR";
    }
}

static std::string errno_bind( int e )
{
    switch (e) {
        case EACCES:         return "EACCES";
        case EADDRINUSE:     return "EADDRINUSE";
        case EADDRNOTAVAIL:  return "EADDRNOTAVAIL";
        case EBADF:          return "EBADF";
        case EINVAL:         return "EINVAL";
#ifdef ENOTSOCK
        case ENOTSOCK:       return "ENOTSOCK";
#endif
        case EFAULT:         return "EFAULT";
        case ENOMEM:         return "ENOMEM";
        default:             return "UNKNOWN_ERR";
    }
}

static std::string errno_listen( int e )
{
    switch (e) {
        case EBADF:         return "EBADF";
#ifdef ENOTSOCK
        case ENOTSOCK:      return "ENOTSOCK";
#endif
#ifdef EOPNOTSUPP
        case EOPNOTSUPP:    return "EOPNOTSUPP";
#endif
        case EINVAL:        return "EINVAL";
        case ENOMEM:        return "ENOMEM";
        default:            return "UNKNOWN_ERR";
    }
}

static std::string errno_poll( int e )
{
    switch (e) {
        case EINVAL:    return "EINVAL";
        case EFAULT:    return "EFAULT";
        case ENOMEM:    return "ENOMEM";
#ifdef EINTR
        case EINTR:     return "EINTR";
#endif
        default:        return "UNKNOWN_ERR";
    }
}

static std::string errno_accept( int e )
{
    switch (e) {
#ifdef EAGAIN
        case EAGAIN:        return "EAGAIN/EWOULDBLOCK";
#endif
        case EINTR:         return "EINTR";
#ifdef ECONNABORTED
        case ECONNABORTED:  return "ECONNABORTED";
#endif
        case EBADF:         return "EBADF";
#ifdef ENOTSOCK
        case ENOTSOCK:      return "ENOTSOCK";
#endif
        case EINVAL:        return "EINVAL";
        case EMFILE:        return "EMFILE";
        case ENFILE:        return "ENFILE";
        case ENOMEM:        return "ENOMEM";
        case EFAULT:        return "EFAULT";
        default:            return "UNKNOWN_ERR";
    }
}

static std::string errno_recv( int e )
{
    switch (e) {
#ifdef EAGAIN
        case EAGAIN:       return "EAGAIN/EWOULDBLOCK";
#endif
        case EINTR:        return "EINTR";
#ifdef ECONNRESET
        case ECONNRESET:   return "ECONNRESET";
#endif
        case EBADF:        return "EBADF";
#ifdef ENOTSOCK
        case ENOTSOCK:     return "ENOTSOCK";
#endif
        case EINVAL:       return "EINVAL";
        case ENOMEM:       return "ENOMEM";
        case EFAULT:       return "EFAULT";
        default:           return "UNKNOWN_ERR";
    }
}

static std::string errno_send( int e )
{
    switch (e) {
#ifdef EAGAIN
        case EAGAIN:       return "EAGAIN/EWOULDBLOCK";
#endif
        case EINTR:        return "EINTR";
#ifdef EPIPE
        case EPIPE:        return "EPIPE";
#endif
#ifdef ECONNRESET
        case ECONNRESET:   return "ECONNRESET";
#endif
#ifdef ENOTCONN
        case ENOTCONN:     return "ENOTCONN";
#endif
        case EBADF:        return "EBADF";
#ifdef ENOTSOCK
        case ENOTSOCK:     return "ENOTSOCK";
#endif
        case EINVAL:       return "EINVAL";
        case ENOMEM:       return "ENOMEM";
        case EFAULT:       return "EFAULT";
        default:           return "UNKNOWN_ERR";
    }
}

static std::string errno_fcntl( int e )
{
    switch (e) {
        case EBADF:  return "EBADF";
        case EINVAL: return "EINVAL";
        case EACCES: return "EACCES";
#ifdef EAGAIN
        case EAGAIN: return "EAGAIN";
#endif
        default:     return "UNKNOWN_ERR";
    }
}

// ------------------------------------------------------------
// function name mapper
// ------------------------------------------------------------
static const char* func_name( ErrFunc func )
{
    switch (func) {
        case EF_SOCKET:     return "socket";
        case EF_SETSOCKOPT: return "setsockopt";
        case EF_BIND:       return "bind";
        case EF_LISTEN:     return "listen";
        case EF_ACCEPT:     return "accept";
        case EF_POLL:       return "poll";
        case EF_RECV:       return "recv";
        case EF_SEND:       return "send";
        case EF_FCNTL:      return "fcntl";
        default:            return "unknown";
    }
}

// ------------------------------------------------------------
// public API
// ------------------------------------------------------------
std::string err_word( int err_no, ErrFunc func )
{
    std::string s(func_name(func));
    s += " Failed: ";

    switch (func) {
        case EF_SOCKET:     s += errno_socket(err_no);      break;
        case EF_SETSOCKOPT: s += errno_setsockopt(err_no);  break;
        case EF_BIND:       s += errno_bind(err_no);        break;
        case EF_LISTEN:     s += errno_listen(err_no);      break;
        case EF_POLL:       s += errno_poll(err_no);        break;
        case EF_ACCEPT:     s += errno_accept(err_no);      break;
        case EF_RECV:       s += errno_recv(err_no);        break;
        case EF_SEND:       s += errno_send(err_no);        break;
        case EF_FCNTL:	    s += errno_fcntl(err_no);	    break;
        default:            s += "UNKNOWN_ERR";             break;
    }
    s += " (";
    s += std::strerror(err_no);
    s += ")";
    return s;
}