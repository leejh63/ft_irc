#include "IrcCore.hpp"
#include "IrcParser.hpp"

#include <iostream>
#include <sstream>

namespace
{
    const bool kEnableDebugLogging = false;
}

static std::string build_cmd_struct_string(
    const ClientEntry& entry,
    const IrcCommand& cmd )
{
    std::ostringstream oss;

    oss << "=========[CMD fd=" << entry.fd << "]=========\n";

    // -------- IrcCommand --------
    oss << "[COMMAND]\n";
    oss << "raw_line=     '" << cmd.raw_Line << "'\n";
    oss << "prefix=       '" << cmd.prefix << "'\n";
    oss << "verb=         '" << cmd.verb << "'\n";

    oss << "params(" << cmd.params.size() << ")=    ";
    for (size_t i = 0; i < cmd.params.size(); ++i)
        oss << "[" << cmd.params[i] << "]";
    oss << "\n";

    oss << "trailing=     '" << cmd.trailing << "'\n";

    // -------- Client --------
    oss << "[IO]\n";
    oss << "fd=           " << entry.fd << "\n";
    oss << "inBuf=       =" << entry.inBuf << "=\n";
    oss << "outBuf=      =" << entry.outBuf << "=\n";

    // -------- Registration / IRC State --------
    oss << "[STATE]\n";
    oss << "passOk=       " << (entry.passOk ? "true" : "false") << "\n";
    oss << "hasNick=      " << (entry.hasNick ? "true" : "false") << "\n";
    oss << "hasUser=      " << (entry.hasUser ? "true" : "false") << "\n";
    oss << "registered=   " << (entry.registered ? "true" : "false") << "\n";

    oss << "nick=         '" << entry.nick << "'\n";
    oss << "user=         '" << entry.user << "'\n";
    oss << "realName=     '" << entry.realName << "'\n";
    oss << "userModes=    '" << entry.userModes << "'\n";

    oss << "===============================\n";

    return oss.str();
}

IrcCore::IrcCore( const std::string& password, ClientRegistry& clients, ChannelRegistry&  channels )
: _server_password(password)
, _clients(clients)
, _channels(channels)
, _messages(clients, channels)
{

}

IrcCore::~IrcCore( void )
{
}

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

void IrcCore::debug_Full( const ClientEntry& entry,
                          const IrcCommand& cmd,
                          const char* msg ) const
{
    if (!kEnableDebugLogging)
        return;

    std::cout << "\n========================================\n";
    _clients.debug_Print_All();
    _channels.debug_Print_All();

    std::cout << build_cmd_struct_string(entry, cmd);
    std::cout << msg;


    std::cout << "========================================\n";
}

const char* IrcCore::debug_Message( handleResult result ) const
{
    switch (result)
    {
        // password
        case PASS_ALREADY_REGISTERED:
            return "[PASS] already registered\n";
        case PASS_PARAM_MISSING:
            return "[PASS] parameter missing\n";
        case PASS_PASSWORD_OK:
            return "[PASS] password same\n";
        case PASS_PASSWORD_BAD:
            return "[PASS] password not same\n";

        // nickname
        case NICK_ERRONEUS:
            return "[NICK] erroneous nickname\n";
        case NICK_PARAM_MISSING:
            return "[NICK] nickname missing\n";
        case NICK_IN_USE:
            return "[NICK] nickname already in use\n";
        case NICK_OK:
            return "[NICK] nickname good\n";

        // user
        case USER_PARAM_MISSING:
            return "[USER] parameter missing\n";
        case USER_REALNAME_MISSING:
            return "[USER] realname missing\n";
        case USER_ALREADY_REGISTERED:
            return "[USER] already registered\n";
        case USER_OK:
            return "[USER] user good\n";

        // etc
        case HANDLE_ERROR:
            return "[ERROR] unknown result\n";
        case HANDLE_UNKNOWN:
            return "[UNKNOWN] unknown command\n";
        default:
            return "[default] unknown result\n";
    }
}

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

void IrcCore::push_Close( std::vector<ServerAction>& out,
                          int fd ) const
{
    ServerAction act;
    act.type = SERVER_ACTION_CLOSE;
    act.fd = fd;
    out.push_back(act);
}


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
