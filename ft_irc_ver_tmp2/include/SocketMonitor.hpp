#ifndef SOCKETMONITOR_HPP
#define SOCKETMONITOR_HPP

#include <cstddef>
#include <vector>

#include <poll.h>

class SocketMonitor
{
public:
    SocketMonitor( void );
    ~SocketMonitor( void );

    void    init( int listenFd );
    int     wait( int timeoutMs );

    size_t  size( void ) const;
    int     fd_At( size_t idx ) const;
    short   revents_At( size_t idx ) const;

    bool    listen_Has_Error( void ) const;
    bool    listen_Can_Accept( void ) const;

    void    add_Client( int fd );
    void    remove_At( size_t idx );

    void    enable_Write( int fd );
    void    disable_Write( int fd );

private:
    int     find_Index_By_Fd( int fd ) const;

private:
    std::vector<pollfd> _fds;

private:
    SocketMonitor( const SocketMonitor& );
    SocketMonitor& operator=( const SocketMonitor& );
};

#endif
