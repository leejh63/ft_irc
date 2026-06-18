#include "IrcCore.hpp"
#include "IrcParser.hpp"


// 서버 비밀번호, 클라이언트/채널 저장소, 메시지 빌더를 연결한다.
IrcCore::IrcCore( const std::string& password, ClientRegistry& clients, ChannelRegistry&  channels )
: _server_password(password)
, _clients(clients)
, _channels(channels)
, _messages(clients, channels)
{

}

// IRC 코어 자원을 정리한다.
IrcCore::~IrcCore( void )
{
}

// 클라이언트 종료 메시지를 공유 채널에 알리고 연결 종료 액션을 만든다.
void IrcCore::disconnect_Client( int fd,
                                 const std::string& reason,
                                 std::vector<ServerAction>& out )
{
    std::set<int> peers;
    _channels.collect_Shared_Peers(fd, peers);

    std::vector<std::string> joinedChannels;
    _channels.collect_User_Channels(fd, joinedChannels);

    if (!peers.empty())
    {
        const std::string quitMsg = _messages.build_Quit_Message(fd, reason);

        std::set<int>::const_iterator pit = peers.begin();
        for (; pit != peers.end(); ++pit)
            push_Send(out, *pit, quitMsg);
    }

    for (size_t i = 0; i < joinedChannels.size(); ++i)
        _channels.remove_Member_And_Cleanup(joinedChannels[i], fd);

    push_Close(out, fd);
}

// 원본 IRC 라인을 파싱한 뒤 적절한 명령 핸들러로 전달한다.
void IrcCore::handle_Line( ClientEntry& entry,
                           const std::string& raw_Line,
                           std::vector<ServerAction>& out )
{
    IrcCommand cmd;

    if (!IrcParser::parse_Line(raw_Line, cmd))
    {
        cmd.raw_Line = raw_Line;
        return handle_Error(entry, cmd, out);
    }

    handle_Command(entry, cmd, out);
}

// 명령 처리 흐름을 추적하기 위한 훅으로, 현재는 출력하지 않는다.
void IrcCore::trace_Full( const ClientEntry& entry,
                          const IrcCommand& cmd,
                          const char* msg ) const
{
    (void)entry;
    (void)cmd;
    (void)msg;
}

// 처리 결과 enum을 추적용 메시지 문자열로 변환한다.
const char* IrcCore::trace_Message( handleResult result ) const
{
    switch (result)
    {
        // PASS 처리 결과
        case PASS_ALREADY_REGISTERED:
            return "[PASS] already registered\n";
        case PASS_PARAM_MISSING:
            return "[PASS] parameter missing\n";
        case PASS_PASSWORD_OK:
            return "[PASS] password same\n";
        case PASS_PASSWORD_BAD:
            return "[PASS] password not same\n";

        // NICK 처리 결과
        case NICK_ERRONEUS:
            return "[NICK] erroneous nickname\n";
        case NICK_PARAM_MISSING:
            return "[NICK] nickname missing\n";
        case NICK_IN_USE:
            return "[NICK] nickname already in use\n";
        case NICK_OK:
            return "[NICK] nickname good\n";

        // USER 처리 결과
        case USER_PARAM_MISSING:
            return "[USER] parameter missing\n";
        case USER_REALNAME_MISSING:
            return "[USER] realname missing\n";
        case USER_ALREADY_REGISTERED:
            return "[USER] already registered\n";
        case USER_OK:
            return "[USER] user good\n";

        // 공통 처리 결과
        case HANDLE_ERROR:
            return "[ERROR] unknown result\n";
        case HANDLE_UNKNOWN:
            return "[UNKNOWN] unknown command\n";
        default:
            return "[default] unknown result\n";
    }
}

// 지정한 클라이언트에게 보낼 메시지 액션을 추가한다.
void IrcCore::push_Send( std::vector<ServerAction>& out,
                         int fd,
                         const std::string& message ) const
{
    ServerAction act;
    act.type = SERVER_ACTION_SEND;
    act.fd = fd;
    act.message = message;
    out.push_back(act);
}

// 지정한 클라이언트를 닫는 액션을 추가한다.
void IrcCore::push_Close( std::vector<ServerAction>& out,
                          int fd ) const
{
    ServerAction act;
    act.type = SERVER_ACTION_CLOSE;
    act.fd = fd;
    out.push_back(act);
}


// 채널 멤버들에게 메시지를 전송하되 필요하면 특정 파일 디스크립터는 제외한다.
void IrcCore::send_To_Channel( const std::string& channelName,
                               const std::string& message,
                               std::vector<ServerAction>& out,
                               int exceptFd ) const
{
    std::vector<int> members;
    _channels.collect_Channel_Members(channelName, members);

    for (size_t i = 0; i < members.size(); ++i)
    {
        if (members[i] == exceptFd)
            continue;
        push_Send(out, members[i], message);
    }
}

// 클라이언트와 채널을 공유하는 모든 피어에게 메시지를 전송한다.
void IrcCore::send_To_Shared_Peers( int fd,
                                    const std::string& message,
                                    std::vector<ServerAction>& out ) const
{
    std::set<int> peers;
    _channels.collect_Shared_Peers(fd, peers);

    std::set<int>::const_iterator it = peers.begin();
    for (; it != peers.end(); ++it)
        push_Send(out, *it, message);
}
