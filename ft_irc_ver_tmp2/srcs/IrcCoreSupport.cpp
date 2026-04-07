#include "IrcCore.hpp"

#include <sstream>

namespace
{
    bool is_Nick_First_Char( char c )
    {
        if (c >= 'A' && c <= 'Z')
            return true;
        if (c >= 'a' && c <= 'z')
            return true;

        if (c == '[' || c == ']' || c == '\\' || c == '`' ||
            c == '_' || c == '^' || c == '{' || c == '|' || c == '}')
            return true;

        return false;
    }

    bool is_Nick_Char( char c )
    {
        if (is_Nick_First_Char(c))
            return true;
        if (c >= '0' && c <= '9')
            return true;
        return c == '-';
    }
}

IrcCore::handleResult IrcCore::check_Pass( int fd,
                                           const IrcCommand& cmd ) const
{
    std::string providedPassword;

    if (_clients.is_Registered(fd) ||
        _clients.has_Nick(fd) ||
        _clients.has_User(fd))
        return PASS_ALREADY_REGISTERED;

    if (!cmd.params.empty())
        providedPassword = cmd.params[0];
    else if (cmd.hasTrailing)
        providedPassword = cmd.trailing;

    if (providedPassword.empty())
        return PASS_PARAM_MISSING;

    if (providedPassword == _server_password)
        return PASS_PASSWORD_OK;

    return PASS_PASSWORD_BAD;
}

IrcCore::handleResult IrcCore::check_Nick( const IrcCommand& cmd ) const
{
    if (cmd.params.empty())
        return NICK_PARAM_MISSING;

    const std::string& nick = cmd.params[0];

    if (nick.empty())
        return NICK_ERRONEUS;

    if (!is_Nick_First_Char(nick[0]))
        return NICK_ERRONEUS;

    for (size_t i = 1; i < nick.size(); ++i)
    {
        if (!is_Nick_Char(nick[i]))
            return NICK_ERRONEUS;
    }

    return NICK_OK;
}

IrcCore::handleResult IrcCore::check_User( int fd,
                                           const IrcCommand& cmd ) const
{
    if (_clients.has_User(fd))
        return USER_ALREADY_REGISTERED;

    if (cmd.params.size() < 3)
        return USER_PARAM_MISSING;

    if (!cmd.hasTrailing)
        return USER_REALNAME_MISSING;

    return USER_OK;
}

IrcCore::handleResult IrcCore::check_Join( const std::string& channelName,
                                           int fd,
                                           const std::string& providedKey ) const
{
    if (_channels.has_Member(channelName, fd))
        return JOIN_ALREADY_MEMBER;

    if (_channels.is_Invite_Only(channelName) &&
        !_channels.is_Invited(channelName, fd))
        return JOIN_INVITE_ONLY;

    if (_channels.has_Key(channelName))
    {
        std::string actualKey;
        if (_channels.get_Key(channelName, actualKey) && actualKey != providedKey)
            return JOIN_BAD_KEY;
    }

    if (_channels.is_Channel_Full(channelName))
        return JOIN_CHANNEL_FULL;

    return JOIN_OK;
}

IrcCore::handleResult IrcCore::apply_Mode_Toggle( const std::string& channelName,
                                                  char sign,
                                                  char modeChar )
{
    const bool enable = (sign == '+');

    if (modeChar == 'i')
    {
        _channels.set_Invite_Only(channelName, enable);
        return MODE_APPLY_OK;
    }

    if (modeChar == 't')
    {
        _channels.set_Topic_Op_Only(channelName, enable);
        return MODE_APPLY_OK;
    }

    return MODE_UNKNOWN_CHAR;
}

IrcCore::handleResult IrcCore::apply_Mode_Param( const std::string& channelName,
                                                 char sign,
                                                 char modeChar,
                                                 const std::string& modeParam,
                                                 std::string& appliedParam )
{
    if (modeChar == 'k')
    {
        if (sign == '+')
        {
            if (modeParam.empty())
                return MODE_PARAM_MISSING;

            _channels.set_Key(channelName, modeParam);
            appliedParam = "*";
            return MODE_APPLY_OK;
        }

        _channels.clear_Key(channelName);
        appliedParam.clear();
        return MODE_APPLY_OK;
    }

    if (modeChar == 'l')
    {
        if (sign == '+')
        {
            if (modeParam.empty())
                return MODE_PARAM_MISSING;

            if (!is_Number_String(modeParam))
                return MODE_BAD_VALUE;

            std::istringstream iss(modeParam);
            int limit = 0;
            iss >> limit;

            if (limit <= 0)
                return MODE_BAD_VALUE;

            _channels.set_User_Limit(channelName, limit);
            appliedParam = modeParam;
            return MODE_APPLY_OK;
        }

        _channels.clear_User_Limit(channelName);
        appliedParam.clear();
        return MODE_APPLY_OK;
    }

    return MODE_UNKNOWN_CHAR;
}

IrcCore::handleResult IrcCore::apply_Mode_Member( const std::string& channelName,
                                                  char sign,
                                                  char modeChar,
                                                  const std::string& modeParam,
                                                  std::string& appliedParam )
{
    if (modeChar != 'o')
        return MODE_UNKNOWN_CHAR;

    if (modeParam.empty())
        return MODE_PARAM_MISSING;

    ClientEntry* target = _clients.find_By_Nick(modeParam);
    if (target == NULL)
        return MODE_NO_SUCH_NICK;

    if (!_channels.has_Member(channelName, target->fd))
        return MODE_USER_NOT_IN_CHANNEL;

    if (sign == '+')
        _channels.add_Operator(channelName, target->fd);
    else
    {
        if (_channels.would_Remove_Last_Operator(channelName, target->fd))
            return MODE_LAST_OPERATOR;
        _channels.remove_Operator(channelName, target->fd);
    }

    appliedParam = modeParam;
    return MODE_APPLY_OK;
}

IrcCore::handleResult IrcCore::apply_Channel_Mode( const std::string& channelName,
                                                   char sign,
                                                   char modeChar,
                                                   const std::string& modeParam,
                                                   std::string& appliedParam )
{
    handleResult result = apply_Mode_Toggle(channelName, sign, modeChar);
    if (result != MODE_UNKNOWN_CHAR)
        return result;

    result = apply_Mode_Param(channelName, sign, modeChar, modeParam, appliedParam);
    if (result != MODE_UNKNOWN_CHAR)
        return result;

    return apply_Mode_Member(channelName, sign, modeChar, modeParam, appliedParam);
}

bool IrcCore::apply_User_Mode( int fd,
                               char sign,
                               char modeChar )
{
    if (!is_Supported_User_Mode(modeChar))
        return false;

    return _clients.set_User_Mode(fd, modeChar, sign == '+');
}

bool IrcCore::mode_Requires_Param( char sign,
                                   char modeChar ) const
{
    if (modeChar == 'o')
        return true;

    if (modeChar == 'k' && sign == '+')
        return true;

    if (modeChar == 'l' && sign == '+')
        return true;

    return false;
}

void IrcCore::append_Mode_Change( std::string& outModes,
                                  std::string& outParams,
                                  char sign,
                                  char modeChar,
                                  const std::string& appliedParam ) const
{
    if (outModes.empty() || outModes[outModes.size() - 1] != sign)
        outModes += sign;

    outModes += modeChar;

    if (!appliedParam.empty())
    {
        if (!outParams.empty())
            outParams += " ";
        outParams += appliedParam;
    }
}

void IrcCore::try_Register( ClientEntry& entry,
                            std::vector<ServerAction>& out )
{
    if (_clients.try_Register(entry.fd))
    {
        push_Send(out, entry.fd,
                  _messages.build_Welcome_Reply(current_Nick(entry.fd)));
    }
}

const IrcCore::CommandRoute* IrcCore::find_Command_Route( const std::string& verb ) const
{
    static const CommandRoute routes[] = {
        {"CAP", &IrcCore::handle_Cap, true},
        {"PING", &IrcCore::handle_Ping, true},
        {"WHO", &IrcCore::handle_Who, false},
        {"PONG", &IrcCore::handle_Pong, true},
        {"PASS", &IrcCore::handle_Pass, true},
        {"NICK", &IrcCore::handle_Nick, true},
        {"USER", &IrcCore::handle_User, true},
        {"JOIN", &IrcCore::handle_Join, false},
        {"PART", &IrcCore::handle_Part, false},
        {"PRIVMSG", &IrcCore::handle_Privmsg, false},
        {"TOPIC", &IrcCore::handle_Topic, false},
        {"INVITE", &IrcCore::handle_Invite, false},
        {"KICK", &IrcCore::handle_Kick, false},
        {"MODE", &IrcCore::handle_Mode, false},
        {"QUIT", &IrcCore::handle_Quit, true},
    };

    const size_t routeCount = sizeof(routes) / sizeof(routes[0]);

    for (size_t i = 0; i < routeCount; ++i)
    {
        if (verb == routes[i].verb)
            return &routes[i];
    }

    return NULL;
}

std::string IrcCore::current_Nick( int fd ) const
{
    return _clients.get_Nick(fd);
}

void IrcCore::reply_And_Debug( const ClientEntry& entry,
                               const IrcCommand& cmd,
                               std::vector<ServerAction>& out,
                               const std::string& reply,
                               const char* debugMessage ) const
{
    push_Send(out, entry.fd, reply);
    debug_Full(entry, cmd, debugMessage);
}

void IrcCore::reply_Need_More_Params( const ClientEntry& entry,
                                      const IrcCommand& cmd,
                                      std::vector<ServerAction>& out,
                                      const std::string& command,
                                      const char* debugMessage ) const
{
    reply_And_Debug(
        entry,
        cmd,
        out,
        _messages.build_Err_Need_more_params(current_Nick(entry.fd), command),
        debugMessage);
}

bool IrcCore::ensure_Pass_Accepted( const ClientEntry& entry,
                                    const IrcCommand& cmd,
                                    std::vector<ServerAction>& out,
                                    const char* debugMessage )
{
    if (_clients.has_Pass_Ok(entry.fd))
        return true;

    reply_And_Debug(entry, cmd, out,
                    _messages.build_Err_Not_registered(current_Nick(entry.fd)),
                    debugMessage);
    return false;
}

bool IrcCore::require_Channel_Exists( const ClientEntry& entry,
                                      const IrcCommand& cmd,
                                      const std::string& channelName,
                                      std::vector<ServerAction>& out,
                                      const char* debugMessage )
{
    if (_channels.has_Channel(channelName))
        return true;

    reply_And_Debug(entry, cmd, out,
                    _messages.build_Err_No_such_channel(current_Nick(entry.fd), channelName),
                    debugMessage);
    return false;
}

bool IrcCore::require_Channel_Member( const ClientEntry& entry,
                                      const IrcCommand& cmd,
                                      const std::string& channelName,
                                      std::vector<ServerAction>& out,
                                      const char* debugMessage )
{
    if (_channels.has_Member(channelName, entry.fd))
        return true;

    reply_And_Debug(entry, cmd, out,
                    _messages.build_Err_Not_on_channel(current_Nick(entry.fd), channelName),
                    debugMessage);
    return false;
}

bool IrcCore::require_Channel_Privilege( const ClientEntry& entry,
                                         const IrcCommand& cmd,
                                         bool allowed,
                                         const std::string& channelName,
                                         std::vector<ServerAction>& out,
                                         const char* debugMessage ) const
{
    if (allowed)
        return true;

    reply_And_Debug(entry, cmd, out,
                    _messages.build_Err_Chan_oprivs_needed(current_Nick(entry.fd), channelName),
                    debugMessage);
    return false;
}

bool IrcCore::require_Target_Channel_Member( const ClientEntry& entry,
                                             const IrcCommand& cmd,
                                             const std::string& channelName,
                                             const std::string& targetNick,
                                             int targetFd,
                                             std::vector<ServerAction>& out,
                                             const char* debugMessage ) const
{
    if (_channels.has_Member(channelName, targetFd))
        return true;

    reply_And_Debug(entry, cmd, out,
                    _messages.build_Err_User_not_in_channel(
                        current_Nick(entry.fd),
                        targetNick,
                        channelName),
                    debugMessage);
    return false;
}

ClientEntry* IrcCore::find_Target_Client( const ClientEntry& entry,
                                          const IrcCommand& cmd,
                                          const std::string& targetNick,
                                          std::vector<ServerAction>& out,
                                          const char* debugMessage )
{
    ClientEntry* targetClient = _clients.find_By_Nick(targetNick);
    if (targetClient != NULL)
        return targetClient;

    reply_And_Debug(entry, cmd, out,
                    _messages.build_Err_No_such_nick(current_Nick(entry.fd), targetNick),
                    debugMessage);
    return NULL;
}

void IrcCore::reply_Mode_Error( const ClientEntry& entry,
                                const IrcCommand& cmd,
                                std::vector<ServerAction>& out,
                                const std::string& channelName,
                                char modeChar,
                                const std::string& modeParam,
                                handleResult modeResult ) const
{
    const std::string nick = current_Nick(entry.fd);

    if (modeResult == MODE_PARAM_MISSING)
    {
        reply_Need_More_Params(entry, cmd, out, "MODE", "[MODE] parameter missing\n");
        return;
    }

    if (modeResult == MODE_NO_SUCH_NICK)
    {
        reply_And_Debug(entry, cmd, out,
                        _messages.build_Err_No_such_nick(nick, modeParam),
                        "[MODE] no such nick\n");
        return;
    }

    if (modeResult == MODE_USER_NOT_IN_CHANNEL)
    {
        reply_And_Debug(entry, cmd, out,
                        _messages.build_Err_User_not_in_channel(nick, modeParam, channelName),
                        "[MODE] target not on channel\n");
        return;
    }

    if (modeResult == MODE_BAD_VALUE)
    {
        reply_And_Debug(entry, cmd, out,
                        _messages.build_Err_Invalid_mode_param(
                            nick,
                            channelName,
                            modeChar,
                            modeParam),
                        "[MODE] bad value\n");
        return;
    }

    if (modeResult == MODE_LAST_OPERATOR)
    {
        reply_And_Debug(entry, cmd, out,
                        _messages.build_Err_Last_operator(nick, channelName),
                        "[MODE] cannot remove last operator\n");
        return;
    }

    reply_And_Debug(entry, cmd, out,
                    _messages.build_Err_Unknown_mode(nick, modeChar),
                    "[MODE] unsupported mode\n");
}

void IrcCore::handle_Command( ClientEntry& entry,
                              const IrcCommand& cmd,
                              std::vector<ServerAction>& out )
{
    const CommandRoute* route = find_Command_Route(cmd.verb);
    if (route == NULL)
        return handle_Unknown(entry, cmd, out);

    if (!_clients.is_Registered(entry.fd) && !route->allowedBeforeRegister)
    {
        reply_And_Debug(entry, cmd, out,
                        _messages.build_Err_Not_registered(current_Nick(entry.fd)),
                        "[ERROR] not registered\n");
        return;
    }

    return (this->*(route->handler))(entry, cmd, out);
}

bool IrcCore::is_Supported_User_Mode( char modeChar ) const
{
    return modeChar == 'i' ||
           modeChar == 'w';
}

bool IrcCore::is_Supported_Channel_Mode( char modeChar ) const
{
    return modeChar == 'i' ||
           modeChar == 't' ||
           modeChar == 'k' ||
           modeChar == 'l' ||
           modeChar == 'o';
}

bool IrcCore::is_Nick_available( int fd,
                                 const std::string& nick ) const
{
    return !_clients.is_Nick_In_Use(nick, fd);
}

bool IrcCore::is_Number_String( const std::string& value ) const
{
    if (value.empty())
        return false;

    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] < '0' || value[i] > '9')
            return false;
    }

    return true;
}
