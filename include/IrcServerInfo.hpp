#ifndef IRCSERVERINFO_HPP
#define IRCSERVERINFO_HPP

class IrcServerInfo
{
public:
    static const char* server_Name( void );
    static const char* host_Name( void );

private:
    IrcServerInfo( void );
    IrcServerInfo( const IrcServerInfo& );
    IrcServerInfo& operator=( const IrcServerInfo& );
};

#endif
