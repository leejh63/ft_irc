#include "SocketMonitor.hpp"

// 비어 있는 poll 대상 목록을 준비한다.
SocketMonitor::SocketMonitor( void )
: _fds()
{
}

// poll 대상 목록의 자원을 정리한다.
SocketMonitor::~SocketMonitor( void )
{
}

// listen 소켓을 첫 번째 poll 대상으로 등록한다.
void SocketMonitor::init( int listenFd )
{
    _fds.clear();

    pollfd listenPollFd;
    listenPollFd.fd = listenFd;
    listenPollFd.events = POLLIN;
    listenPollFd.revents = 0;

    _fds.push_back(listenPollFd);
}

// 등록된 파일 디스크립터들에 대해 poll을 수행한다.
int SocketMonitor::wait( int timeoutMs )
{
    if (_fds.empty())
        return 0;

    return poll(&_fds[0], _fds.size(), timeoutMs);
}

// 현재 감시 중인 파일 디스크립터 개수를 반환한다.
size_t SocketMonitor::size( void ) const
{
    return _fds.size();
}

// 지정한 인덱스의 파일 디스크립터를 반환한다.
int SocketMonitor::fd_At( size_t idx ) const
{
    return _fds[idx].fd;
}

// 지정한 인덱스의 poll 이벤트 결과를 반환한다.
short SocketMonitor::revents_At( size_t idx ) const
{
    return _fds[idx].revents;
}

// listen 소켓에 오류나 연결 종료 이벤트가 있는지 확인한다.
bool SocketMonitor::listen_Has_Error( void ) const
{
    if (_fds.empty())
        return false;

    const short revents = _fds[0].revents;
    return (revents & (POLLERR | POLLHUP | POLLNVAL)) != 0;
}

// listen 소켓이 새 연결을 받을 수 있는 상태인지 확인한다.
bool SocketMonitor::listen_Can_Accept( void ) const
{
    if (_fds.empty())
        return false;

    return (_fds[0].revents & POLLIN) != 0;
}

// 새 클라이언트 소켓을 읽기 감시 대상으로 추가한다.
void SocketMonitor::add_Client( int fd )
{
    pollfd clientPollFd;
    clientPollFd.fd = fd;
    clientPollFd.events = POLLIN;
    clientPollFd.revents = 0;

    _fds.push_back(clientPollFd);
}

// 지정한 인덱스의 poll 대상을 제거한다.
void SocketMonitor::remove_At( size_t idx )
{
    _fds.erase(_fds.begin() + idx);
}

// 지정한 파일 디스크립터에 쓰기 가능 이벤트 감시를 켠다.
void SocketMonitor::enable_Write( int fd )
{
    const int idx = find_Index_By_Fd(fd);
    if (idx < 0)
        return;

    _fds[idx].events |= POLLOUT;
}

// 지정한 파일 디스크립터의 쓰기 가능 이벤트 감시를 끈다.
void SocketMonitor::disable_Write( int fd )
{
    const int idx = find_Index_By_Fd(fd);
    if (idx < 0)
        return;

    _fds[idx].events &= ~POLLOUT;
}

// 파일 디스크립터로 poll 대상 인덱스를 찾는다.
int SocketMonitor::find_Index_By_Fd( int fd ) const
{
    for (size_t i = 0; i < _fds.size(); ++i)
    {
        if (_fds[i].fd == fd)
            return static_cast<int>(i);
    }

    return -1;
}
