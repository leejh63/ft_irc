#include "IrcServerInfo.hpp"

// IRC 응답에 사용할 서버 이름을 반환한다.
const char* IrcServerInfo::server_Name( void )
{
    return "irc.local";
}

// IRC 사용자 접두부에 사용할 호스트 이름을 반환한다.
const char* IrcServerInfo::host_Name( void )
{
    return "localhost";
}
