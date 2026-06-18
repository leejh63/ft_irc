#include "Server.hpp"
#include "Signal.hpp"
#include "Error.hpp"

#include <stdexcept>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <set>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


namespace
{
    const char* kConnectionClosedReason = "Connection closed";
    const char* kSendQueueFullReason = "Send queue full";

    // 파일 디스크립터를 논블로킹 모드로 전환한다.
    void set_Non_Blocking( int fd )
    {
        if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
        {
            const int e = errno;
            throw std::runtime_error(err_word(e, EF_FCNTL));
        }
    }
}

// 서버 상태 추적용 훅으로, 현재는 실제 로그를 남기지 않는다.
void Server::trace_State( const std::string& msg, int fd ) const
{
    (void)msg;
    (void)fd;
}

// 포트와 비밀번호를 받아 서버 구성 요소들을 초기화한다.
Server::Server( int port, const std::string& password )
: _port(port)
, _listenFd()
, _running(false)
, _monitor()
, _clientRegistry()
, _channelRegistry()
, _core(password, _clientRegistry, _channelRegistry)
{
}

// 남아 있는 클라이언트 소켓을 닫아 서버 자원을 정리한다.
Server::~Server( void )
{
    for (size_t i = 1; i < _monitor.size(); ++i)
    {
        const int fd = _monitor.fd_At(i);
        if (fd >= 0)
            close(fd);
    }
}

// listen 소켓을 생성하고 논블로킹으로 설정한다.
void Server::init_Socket( void )
{
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        const int e = errno;
        throw std::runtime_error(err_word(e, EF_SOCKET));
    }

    _listenFd.reset(fd);
    set_Non_Blocking(_listenFd.get());
}

// listen 소켓에 주소 재사용 옵션을 설정한다.
void Server::init_Setsockopt( void )
{
    int reuse = 1;

    if (setsockopt(_listenFd.get(), SOL_SOCKET, SO_REUSEADDR,
                   &reuse, sizeof(reuse)) < 0)
    {
        const int e = errno;
        throw std::runtime_error(err_word(e, EF_SETSOCKOPT));
    }
}

// listen 소켓을 요청받은 포트에 바인딩한다.
void Server::init_Bind( void )
{
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(_port));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(_listenFd.get(), (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        const int e = errno;
        throw std::runtime_error(err_word(e, EF_BIND));
    }
}

// listen 소켓을 연결 대기 상태로 전환한다.
void Server::init_Listen( void )
{
    if (listen(_listenFd.get(), SOMAXCONN) < 0)
    {
        const int e = errno;
        throw std::runtime_error(err_word(e, EF_LISTEN));
    }
}

// SocketMonitor에 listen 소켓을 등록한다.
void Server::init_Monitor( void )
{
    _monitor.init(_listenFd.get());
}

// errno 값이 논블로킹 대기 상태를 뜻하는지 확인한다.
bool Server::is_Would_Block( int e )
{
#ifdef EWOULDBLOCK
    return (e == EAGAIN || e == EWOULDBLOCK);
#else
    return (e == EAGAIN);
#endif
}

// poll이 알린 새 연결 하나를 accept로 받아 클라이언트로 등록한다.
void Server::accept_Pending_Clients( void )
{
    struct sockaddr_in clientAddr;
    socklen_t          clientAddrLen = sizeof(clientAddr);

    // poll은 레벨 트리거 방식이므로 한 번에 연결 하나만 받아도 남은 연결이 있으면
    // listen 파일 디스크립터의 POLLIN이 유지되어 다음 루프에서 다시 처리된다.
    const int clientFd = accept(_listenFd.get(),
                                (struct sockaddr*)&clientAddr,
                                &clientAddrLen);
    if (clientFd < 0)
        return;

    try
    {
        set_Non_Blocking(clientFd);

        if (!_clientRegistry.add_Client(clientFd))
            throw std::runtime_error("client registry add failed");

        _monitor.add_Client(clientFd);
    }
    catch (const std::exception& e)
    {
        _clientRegistry.remove_Client(clientFd);
        close(clientFd);
    }
}

// 파일 디스크립터로 SocketMonitor의 클라이언트 인덱스를 찾는다.
size_t Server::find_Client_Index_By_Fd( int fd ) const
{
    for (size_t i = 1; i < _monitor.size(); ++i)
    {
        if (_monitor.fd_At(i) == fd)
            return i;
    }

    return _monitor.size();
}

// 클라이언트 소켓을 닫고 관련 레지스트리와 채널 상태를 정리한다.
bool Server::close_Client( size_t idx )
{
    const int fd = _monitor.fd_At(idx);

    _channelRegistry.remove_Client_From_All_Channels(fd);

    if (fd >= 0)
        close(fd);

    _clientRegistry.remove_Client(fd);
    _monitor.remove_At(idx);

    trace_State("[Server::close_Client] after channel cleanup", fd);
    return true;
}

// 파일 디스크립터로 클라이언트를 찾아 닫는다.
bool Server::close_Client_By_Fd( int fd )
{
    const size_t idx = find_Client_Index_By_Fd(fd);
    if (idx >= _monitor.size())
        return false;

    return close_Client(idx);
}

// IrcCore가 만든 서버 액션들을 출력 큐 또는 연결 종료로 반영한다.
bool Server::dispatch_Actions( int sourceFd,
                               const std::vector<ServerAction>& actions )
{
    bool removedClient = false;
    bool shouldCloseSource = false;
    std::set<int> saturatedClients;

    for (size_t i = 0; i < actions.size(); ++i)
    {
        if (actions[i].type == SERVER_ACTION_SEND)
        {
            const EnqueueResult result = enqueue(actions[i].fd, actions[i].message);

            if (result == ENQUEUE_BUFFER_FULL)
            {
                saturatedClients.insert(actions[i].fd);
                continue;
            }
            continue;
        }

        if (actions[i].type == SERVER_ACTION_CLOSE && actions[i].fd == sourceFd)
            shouldCloseSource = true;
    }

    for (std::set<int>::const_iterator it = saturatedClients.begin();
         it != saturatedClients.end();
         ++it)
    {
        if (disconnect_Client_By_Fd(*it, kSendQueueFullReason))
            removedClient = true;
    }

    if (shouldCloseSource && close_Client_By_Fd(sourceFd))
        removedClient = true;

    return removedClient;
}

// 지정한 인덱스의 클라이언트를 IRC 종료 절차를 거쳐 연결 해제한다.
bool Server::disconnect_Client( size_t idx,
                                const std::string& reason )
{
    return disconnect_Client_By_Fd(_monitor.fd_At(idx), reason);
}

// 파일 디스크립터로 클라이언트를 찾아 IRC 종료 절차를 수행한다.
bool Server::disconnect_Client_By_Fd( int fd,
                                      const std::string& reason )
{
    if (find_Client_Index_By_Fd(fd) >= _monitor.size())
        return false;

    std::vector<ServerAction> actions;
    _core.disconnect_Client(fd, reason, actions);
    return dispatch_Actions(fd, actions);
}

// 클라이언트 출력 버퍼에 메시지를 추가할 수 있는지 확인하고 저장한다.
Server::EnqueueResult Server::append_To_Output_Buffer( int fd,
                                                       const std::string& msg )
{
    ClientEntry* entry = _clientRegistry.find_By_Fd(fd);
    if (entry == NULL)
        return ENQUEUE_NO_TARGET;

    if (msg.size() > (MAX_OUTBUF - entry->outBuf.size()))
        return ENQUEUE_BUFFER_FULL;

    entry->outBuf += msg;
    return ENQUEUE_OK;
}

// 메시지를 출력 버퍼에 넣고 쓰기 이벤트 감시를 켠다.
Server::EnqueueResult Server::enqueue( int fd, const std::string& msg )
{
    const EnqueueResult result = append_To_Output_Buffer(fd, msg);
    if (result != ENQUEUE_OK)
        return result;

    _monitor.enable_Write(fd);
    return ENQUEUE_OK;
}

// 입력 버퍼에서 IRC 한 줄을 추출한다.
bool Server::extract_Line( std::string& buf, std::string& line )
{
    std::string::size_type pos = buf.find("\r\n");
    if (pos != std::string::npos)
    {
        line = buf.substr(0, pos);
        buf.erase(0, pos + 2);
        return true;
    }

    pos = buf.find('\n');
    if (pos != std::string::npos)
    {
        line = buf.substr(0, pos);
        buf.erase(0, pos + 1);

        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        return true;
    }

    return false;
}

// 클라이언트 소켓에서 데이터를 읽고 완성된 IRC 라인을 처리한다.
bool Server::read_From_Client( size_t idx )
{
    const int    fd = _monitor.fd_At(idx);
    ClientEntry* entry = _clientRegistry.find_By_Fd(fd);
    if (entry == NULL)
    {
        return close_Client(idx);
    }

    char buf[MAX_INBUF];

    // poll은 레벨 트리거 방식이므로 한 번만 recv해도 남은 데이터가 있으면
    // POLLIN이 유지되어 다음 루프에서 다시 읽는다.
    const ssize_t bytes = recv(fd, buf, sizeof(buf), 0);

    if (bytes == 0)
        return disconnect_Client(idx, kConnectionClosedReason);

    if (bytes < 0)
        return false;

    entry->inBuf.append(buf, bytes);

    if (entry->inBuf.size() > MAX_INBUF)
        return disconnect_Client(idx, kConnectionClosedReason);

    std::string line;
    while (extract_Line(entry->inBuf, line))
    {
        if (line.size() > MAX_IRC_LINE)
            return disconnect_Client(idx, kConnectionClosedReason);

        std::vector<ServerAction> actions;
        _core.handle_Line(*entry, line, actions);

        if (dispatch_Actions(fd, actions))
            return true;
    }

    return false;
}

// 클라이언트 출력 버퍼의 데이터를 소켓으로 전송한다.
bool Server::flush_Client_Output( size_t idx )
{
    const int    fd = _monitor.fd_At(idx);
    ClientEntry* entry = _clientRegistry.find_By_Fd(fd);
    if (entry == NULL)
    {
        return close_Client(idx);
    }

    if (entry->outBuf.empty())
    {
        _monitor.disable_Write(fd);
        return false;
    }

    // POLLOUT 한 번에 send 한 번만 수행한다. 커널 버퍼가 가득 차 남은 데이터가
    // 있으면 쓰기 감시가 유지되어 다음 poll에서 이어서 전송한다.
    const ssize_t bytes = send(fd,
                               entry->outBuf.c_str(),
                               entry->outBuf.size(),
                               0);

    if (bytes == 0)
        return disconnect_Client(idx, kConnectionClosedReason);

    if (bytes < 0)
        return false;

    entry->outBuf.erase(0, static_cast<size_t>(bytes));

    if (entry->outBuf.empty())
        _monitor.disable_Write(fd);

    return false;
}

// 준비된 클라이언트 이벤트 하나를 읽기, 쓰기, 종료 순서로 처리한다.
bool Server::process_Ready_Client( size_t idx )
{
    const short revents = _monitor.revents_At(idx);

    if (revents == 0)
        return false;

    if (revents & (POLLERR | POLLHUP | POLLNVAL))
        return disconnect_Client(idx, kConnectionClosedReason);

    if ((revents & POLLIN) && read_From_Client(idx))
        return true;

    if ((revents & POLLOUT) && flush_Client_Output(idx))
        return true;

    return false;
}

// poll 결과가 있는 모든 클라이언트 이벤트를 처리한다.
void Server::process_Ready_Clients( void )
{
    for (size_t i = 1; i < _monitor.size(); )
    {
        if (process_Ready_Client(i))
            continue;
        ++i;
    }
}

// 서버 메인 루프를 실행하며 새 연결과 클라이언트 이벤트를 처리한다.
void Server::run( void )
{
    init_Monitor();
    _running = true;

    while (_running)
    {
        if (Signal::getFlag())
        {
            _running = false;
            break;
        }

        const int readyCount = _monitor.wait(-1);
        if (readyCount < 0)
        {
            const int e = errno;
            if (e == EINTR)
                continue;
            throw std::runtime_error(err_word(e, EF_POLL));
        }

        if (readyCount == 0)
            continue;

        if (_monitor.listen_Has_Error())
            throw std::runtime_error("listen fd poll error");

        if (_monitor.listen_Can_Accept())
            accept_Pending_Clients();

        process_Ready_Clients();
    }
}

// listen 소켓 생성부터 listen 호출까지 서버 시작 준비를 수행한다.
void Server::initialize( void )
{
    init_Socket();
    init_Setsockopt();
    init_Bind();
    init_Listen();
}
