#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>

#include "Fd.hpp"
#include "IrcCore.hpp"
#include "SocketMonitor.hpp"
#include "ClientRegistry.hpp"
#include "ChannelRegistry.hpp"

class Server
{
private:
    enum EnqueueResult
    {
        ENQUEUE_OK,
        ENQUEUE_NO_TARGET,
        ENQUEUE_BUFFER_FULL
    };

private:
    static const size_t MAX_INBUF = 4 * 1024;
    static const size_t MAX_OUTBUF = 64 * 1024;
    static const size_t MAX_IRC_LINE = 510;

private:
    int             _port;
    Fd              _listenFd;
    bool            _running;
    SocketMonitor   _monitor;

    ClientRegistry  _clientRegistry;
    ChannelRegistry _channelRegistry;
    IrcCore         _core;

public:
    Server( int port, const std::string& password );
    ~Server( void );

    void    initialize( void );
    void    run( void );

private:
    void    debug_State( const std::string& msg, int fd ) const;

    void    init_Socket( void );
    void    init_Setsockopt( void );
    void    init_Bind( void );
    void    init_Listen( void );
    void    init_Monitor( void );

    void    accept_Pending_Clients( void );
    bool    close_Client( size_t idx );
    bool    close_Client_By_Fd( int fd );
    bool    disconnect_Client( size_t idx, const std::string& reason );
    bool    disconnect_Client_By_Fd( int fd, const std::string& reason );
    bool    dispatch_Actions( int sourceFd,
                              const std::vector<ServerAction>& actions );
    size_t  find_Client_Index_By_Fd( int fd ) const;

    bool    read_From_Client( size_t idx );
    bool    extract_Line( std::string& buf, std::string& line );

    bool            flush_Client_Output( size_t idx );
    EnqueueResult   append_To_Output_Buffer( int fd, const std::string& msg );
    EnqueueResult   enqueue( int fd, const std::string& msg );

    bool    process_Ready_Client( size_t idx );
    void    process_Ready_Clients( void );

    static bool is_Would_Block( int e );

private:
    Server( void );
    Server( const Server& );
    Server& operator=( const Server& );
};

#endif
