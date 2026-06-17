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

    void set_Non_Blocking( int fd )
    {
        if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
        {
            const int e = errno;
            throw std::runtime_error(err_word(e, EF_FCNTL));
        }
    }
}

void Server::trace_State( const std::string& msg, int fd ) const
{
    (void)msg;
    (void)fd;
}

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

Server::~Server( void )
{
    for (size_t i = 1; i < _monitor.size(); ++i)
    {
        const int fd = _monitor.fd_At(i);
        if (fd >= 0)
            close(fd);
    }
}

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

void Server::init_Listen( void )
{
    if (listen(_listenFd.get(), SOMAXCONN) < 0)
    {
        const int e = errno;
        throw std::runtime_error(err_word(e, EF_LISTEN));
    }
}

void Server::init_Monitor( void )
{
    _monitor.init(_listenFd.get());
}

bool Server::is_Would_Block( int e )
{
#ifdef EWOULDBLOCK
    return (e == EAGAIN || e == EWOULDBLOCK);
#else
    return (e == EAGAIN);
#endif
}

void Server::accept_Pending_Clients( void )
{
    struct sockaddr_in clientAddr;
    socklen_t          clientAddrLen = sizeof(clientAddr);

    // Accept a single connection per poll() notification. poll() is
    // level-triggered, so if more connections are queued, POLLIN on the
    // listen fd stays set and we are called again next iteration.
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

size_t Server::find_Client_Index_By_Fd( int fd ) const
{
    for (size_t i = 1; i < _monitor.size(); ++i)
    {
        if (_monitor.fd_At(i) == fd)
            return i;
    }

    return _monitor.size();
}

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

bool Server::close_Client_By_Fd( int fd )
{
    const size_t idx = find_Client_Index_By_Fd(fd);
    if (idx >= _monitor.size())
        return false;

    return close_Client(idx);
}

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

bool Server::disconnect_Client( size_t idx,
                                const std::string& reason )
{
    return disconnect_Client_By_Fd(_monitor.fd_At(idx), reason);
}

bool Server::disconnect_Client_By_Fd( int fd,
                                      const std::string& reason )
{
    if (find_Client_Index_By_Fd(fd) >= _monitor.size())
        return false;

    std::vector<ServerAction> actions;
    _core.disconnect_Client(fd, reason, actions);
    return dispatch_Actions(fd, actions);
}

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

Server::EnqueueResult Server::enqueue( int fd, const std::string& msg )
{
    const EnqueueResult result = append_To_Output_Buffer(fd, msg);
    if (result != ENQUEUE_OK)
        return result;

    _monitor.enable_Write(fd);
    return ENQUEUE_OK;
}

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

bool Server::read_From_Client( size_t idx )
{
    const int    fd = _monitor.fd_At(idx);
    ClientEntry* entry = _clientRegistry.find_By_Fd(fd);
    if (entry == NULL)
    {
        return close_Client(idx);
    }

    char buf[MAX_INBUF];

    // Single recv() per poll() notification. poll() is level-triggered:
    // if the socket still holds unread data, POLLIN stays set and we are
    // called again. This avoids ever inspecting errno after recv().
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

    // Single send() per POLLOUT notification. If the kernel buffer cannot
    // take everything, POLLOUT stays armed and poll() signals us again;
    // a dead peer surfaces through POLLHUP/POLLERR (handled before us) or
    // a 0-byte recv, so we never need to inspect errno after send().
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

void Server::process_Ready_Clients( void )
{
    for (size_t i = 1; i < _monitor.size(); )
    {
        if (process_Ready_Client(i))
            continue;
        ++i;
    }
}

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

void Server::initialize( void )
{
    init_Socket();
    init_Setsockopt();
    init_Bind();
    init_Listen();
}