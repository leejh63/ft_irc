#include "SocketMonitor.hpp"

SocketMonitor::SocketMonitor( void )
: _fds()
{
}

SocketMonitor::~SocketMonitor( void )
{
}

void SocketMonitor::init( int listenFd )
{
    _fds.clear();

    pollfd listenPollFd;
    listenPollFd.fd = listenFd;
    listenPollFd.events = POLLIN;
    listenPollFd.revents = 0;

    _fds.push_back(listenPollFd);
}

int SocketMonitor::wait( int timeoutMs )
{
    if (_fds.empty())
        return 0;

    return poll(&_fds[0], _fds.size(), timeoutMs);
}

size_t SocketMonitor::size( void ) const
{
    return _fds.size();
}

int SocketMonitor::fd_At( size_t idx ) const
{
    return _fds[idx].fd;
}

short SocketMonitor::revents_At( size_t idx ) const
{
    return _fds[idx].revents;
}

bool SocketMonitor::listen_Has_Error( void ) const
{
    if (_fds.empty())
        return false;

    const short revents = _fds[0].revents;
    return (revents & (POLLERR | POLLHUP | POLLNVAL)) != 0;
}

bool SocketMonitor::listen_Can_Accept( void ) const
{
    if (_fds.empty())
        return false;

    return (_fds[0].revents & POLLIN) != 0;
}

void SocketMonitor::add_Client( int fd )
{
    pollfd clientPollFd;
    clientPollFd.fd = fd;
    clientPollFd.events = POLLIN;
    clientPollFd.revents = 0;

    _fds.push_back(clientPollFd);
}

void SocketMonitor::remove_At( size_t idx )
{
    _fds.erase(_fds.begin() + idx);
}

void SocketMonitor::enable_Write( int fd )
{
    const int idx = find_Index_By_Fd(fd);
    if (idx < 0)
        return;

    _fds[idx].events |= POLLOUT;
}

void SocketMonitor::disable_Write( int fd )
{
    const int idx = find_Index_By_Fd(fd);
    if (idx < 0)
        return;

    _fds[idx].events &= ~POLLOUT;
}

int SocketMonitor::find_Index_By_Fd( int fd ) const
{
    for (size_t i = 0; i < _fds.size(); ++i)
    {
        if (_fds[i].fd == fd)
            return static_cast<int>(i);
    }

    return -1;
}
