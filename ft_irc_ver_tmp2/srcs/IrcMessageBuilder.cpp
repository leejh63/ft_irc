#include "IrcMessageBuilder.hpp"
#include "IrcServerInfo.hpp"

#include <sstream>

namespace
{
    void append_Param( std::string& buffer, const std::string& param )
    {
        if (param.empty())
            return;

        if (!buffer.empty())
            buffer += " ";
        buffer += param;
    }
}

IrcMessageBuilder::IrcMessageBuilder( ClientRegistry& clients,
                                      ChannelRegistry& channels )
: _clients(clients)
, _channels(channels)
{
}

IrcMessageBuilder::~IrcMessageBuilder( void )
{
}

std::string IrcMessageBuilder::normalize_Target( const std::string& value ) const
{
    return value.empty() ? "*" : value;
}

std::string IrcMessageBuilder::build_Server_Prefix( void ) const
{
    return std::string(":") + IrcServerInfo::server_Name();
}

std::string IrcMessageBuilder::nick_Or_star( int fd ) const
{
    return normalize_Target(_clients.get_Nick(fd));
}

std::string IrcMessageBuilder::build_Reply( const std::string& code,
                                            const std::string& target,
                                            const std::string& params,
                                            const std::string& text ) const
{
    std::string msg = build_Server_Prefix();
    msg += " ";
    msg += code;
    msg += " ";
    msg += normalize_Target(target);

    if (!params.empty())
    {
        msg += " ";
        msg += params;
    }

    if (!text.empty())
    {
        msg += " :";
        msg += text;
    }

    msg += "\r\n";
    return msg;
}

std::string IrcMessageBuilder::build_Welcome_Reply( const std::string& nick ) const
{
    const std::string target = normalize_Target(nick);

    return build_Reply(
        "001",
        target,
        "",
        "Welcome to the Internet Relay Network " + target
    );
}

std::string IrcMessageBuilder::build_Err_Need_more_params( const std::string& nick,
                                                           const std::string& command ) const
{
    return build_Reply("461", nick, command, "Not enough parameters");
}

std::string IrcMessageBuilder::build_Err_Already_registered( const std::string& nick ) const
{
    return build_Reply("462", nick, "", "You may not reregister");
}

std::string IrcMessageBuilder::build_Err_Password_mismatch( const std::string& nick ) const
{
    return build_Reply("464", nick, "", "Password incorrect");
}

std::string IrcMessageBuilder::build_Err_Unknown_command( const std::string& nick,
                                                          const std::string& command ) const
{
    return build_Reply("421", nick, command, "Unknown command");
}

std::string IrcMessageBuilder::build_Err_Not_registered( const std::string& nick ) const
{
    return build_Reply("451", nick, "", "You have not registered");
}

std::string IrcMessageBuilder::build_Err_Erroneus_nickname( const std::string& nick,
                                                            const std::string& badNick ) const
{
    return build_Reply("432", nick, badNick, "Erroneous nickname");
}

std::string IrcMessageBuilder::build_Err_Nickname_in_use( const std::string& nick,
                                                          const std::string& badNick ) const
{
    return build_Reply("433", nick, badNick, "Nickname is already in use");
}

std::string IrcMessageBuilder::build_Err_No_recipient( const std::string& nick,
                                                       const std::string& command ) const
{
    return build_Reply("411", nick, "", "No recipient given (" + command + ")");
}

std::string IrcMessageBuilder::build_Err_No_text_to_send( const std::string& nick ) const
{
    return build_Reply("412", nick, "", "No text to send");
}

std::string IrcMessageBuilder::build_Err_No_such_nick( const std::string& nick,
                                                       const std::string& targetNick ) const
{
    return build_Reply("401", nick, targetNick, "No such nick/channel");
}

std::string IrcMessageBuilder::build_Err_No_such_channel( const std::string& nick,
                                                          const std::string& channelName ) const
{
    return build_Reply("403", nick, channelName, "No such channel");
}

std::string IrcMessageBuilder::build_Err_Cannot_send_to_chan( const std::string& nick,
                                                              const std::string& channelName ) const
{
    return build_Reply("404", nick, channelName, "Cannot send to channel");
}

std::string IrcMessageBuilder::build_Err_Not_on_channel( const std::string& nick,
                                                         const std::string& channelName ) const
{
    return build_Reply("442", nick, channelName, "You're not on that channel");
}

std::string IrcMessageBuilder::build_Err_Chan_oprivs_needed( const std::string& nick,
                                                             const std::string& channelName ) const
{
    return build_Reply("482", nick, channelName, "You're not channel operator");
}

std::string IrcMessageBuilder::build_Err_User_on_channel( const std::string& nick,
                                                          const std::string& targetNick,
                                                          const std::string& channelName ) const
{
    return build_Reply("443", nick,
                       targetNick + " " + channelName,
                       "is already on channel");
}

std::string IrcMessageBuilder::build_Err_User_not_in_channel( const std::string& nick,
                                                              const std::string& targetNick,
                                                              const std::string& channelName ) const
{
    return build_Reply("441", nick,
                       targetNick + " " + channelName,
                       "They aren't on that channel");
}

std::string IrcMessageBuilder::build_Err_Invite_only_chan( const std::string& nick,
                                                           const std::string& channelName ) const
{
    return build_Reply("473", nick, channelName, "Cannot join channel (+i)");
}

std::string IrcMessageBuilder::build_Err_Channel_is_full( const std::string& nick,
                                                          const std::string& channelName ) const
{
    return build_Reply("471", nick, channelName, "Cannot join channel (+l)");
}

std::string IrcMessageBuilder::build_Err_Bad_channel_key( const std::string& nick,
                                                          const std::string& channelName ) const
{
    return build_Reply("475", nick, channelName, "Cannot join channel (+k)");
}

std::string IrcMessageBuilder::build_Err_Unknown_mode( const std::string& nick,
                                                       char modeChar ) const
{
    return build_Reply("472", nick, std::string(1, modeChar),
                       "is unknown mode char to me");
}

std::string IrcMessageBuilder::build_Err_Invalid_mode_param( const std::string& nick,
                                                             const std::string& channelName,
                                                             char modeChar,
                                                             const std::string& param ) const
{
    return build_Reply(
        "696",
        nick,
        channelName + " " + std::string(1, modeChar) + " " + param,
        "Invalid mode parameter"
    );
}

std::string IrcMessageBuilder::build_Err_Last_operator( const std::string& nick,
                                                        const std::string& channelName ) const
{
    return build_Reply("482", nick, channelName,
                       "Cannot remove the last channel operator");
}

std::string IrcMessageBuilder::build_Err_Umode_unknown_flag( const std::string& nick ) const
{
    return build_Reply("501", nick, "", "Unknown MODE flag");
}

std::string IrcMessageBuilder::build_Err_Users_dont_match( const std::string& nick ) const
{
    return build_Reply("502", nick, "", "Cannot change mode for other users");
}

std::string IrcMessageBuilder::build_Rpl_Who_reply( const std::string& requesterNick,
                                                    const std::string& channelName,
                                                    int targetFd ) const
{
    if (!_clients.has_Client(targetFd))
        return "";

    std::string nick = _clients.get_Nick(targetFd);
    std::string user = _clients.get_User(targetFd);
    std::string realName = _clients.get_Real_Name(targetFd);

    if (nick.empty())
        nick = "*";
    if (user.empty())
        user = "*";
    if (realName.empty())
        realName = "*";

    std::string flags = "H";
    if (_channels.is_Operator(channelName, targetFd))
        flags += "@";

    return build_Reply(
        "352",
        requesterNick,
        channelName + " " + user + " " + IrcServerInfo::host_Name() + " "
        + IrcServerInfo::server_Name() + " " + nick + " " + flags,
        "0 " + realName
    );
}

std::string IrcMessageBuilder::build_Rpl_End_of_who( const std::string& requesterNick,
                                                     const std::string& name ) const
{
    return build_Reply("315", requesterNick, name, "End of /WHO list");
}

std::string IrcMessageBuilder::build_Rpl_User_mode_is( const std::string& nick,
                                                       const std::string& modes ) const
{
    return build_Reply("221", nick, modes.empty() ? "+" : "+" + modes, "");
}

std::string IrcMessageBuilder::build_Rpl_Channel_mode_is( const std::string& nick,
                                                          const std::string& channelName ) const
{
    std::string modes = "+";
    std::string params;

    if (_channels.is_Invite_Only(channelName))
        modes += "i";

    if (_channels.is_Topic_Op_Only(channelName))
        modes += "t";

    if (_channels.has_Key(channelName))
    {
        std::string key;
        if (_channels.get_Key(channelName, key))
        {
            modes += "k";
            append_Param(params, "*");
        }
    }

    if (_channels.has_User_Limit(channelName))
    {
        size_t limit = 0;
        if (_channels.get_User_Limit(channelName, limit))
        {
            std::ostringstream oss;
            oss << limit;

            modes += "l";
            append_Param(params, oss.str());
        }
    }

    std::string replyParams = channelName + " " + modes;
    append_Param(replyParams, params);

    return build_Reply("324", nick, replyParams, "");
}

std::string IrcMessageBuilder::build_Rpl_No_topic( const std::string& nick,
                                                   const std::string& channelName ) const
{
    return build_Reply("331", nick, channelName, "No topic is set");
}

std::string IrcMessageBuilder::build_Rpl_Topic( const std::string& nick,
                                                const std::string& channelName,
                                                const std::string& topic ) const
{
    return build_Reply("332", nick, channelName, topic);
}

std::string IrcMessageBuilder::build_Rpl_Inviting( const std::string& nick,
                                                   const std::string& targetNick,
                                                   const std::string& channelName ) const
{
    return build_Reply("341", nick, targetNick + " " + channelName, "");
}

std::string IrcMessageBuilder::build_Rpl_Name_reply( const std::string& nick,
                                                     const std::string& channelName ) const
{
    return build_Reply("353", nick, "= " + channelName,
                       build_Channel_Names(channelName));
}

std::string IrcMessageBuilder::build_Rpl_End_of_names( const std::string& nick,
                                                       const std::string& channelName ) const
{
    return build_Reply("366", nick, channelName, "End of /NAMES list");
}

std::string IrcMessageBuilder::build_User_Prefix( int fd ) const
{
    if (!_clients.has_Client(fd))
        return std::string("*!*@") + IrcServerInfo::host_Name();

    std::string nick = _clients.get_Nick(fd);
    std::string user = _clients.get_User(fd);

    if (nick.empty())
        nick = "*";
    if (user.empty())
        user = "*";

    return nick + "!" + user + "@" + IrcServerInfo::host_Name();
}

std::string IrcMessageBuilder::build_User_Command( int fd,
                                                   const std::string& command,
                                                   const std::string& params,
                                                   const std::string& trailing ) const
{
    std::string msg = ":" + build_User_Prefix(fd) + " " + command;

    if (!params.empty())
    {
        msg += " ";
        msg += params;
    }

    if (!trailing.empty())
    {
        msg += " :";
        msg += trailing;
    }

    msg += "\r\n";
    return msg;
}

std::string IrcMessageBuilder::build_Join_Message( int fd,
                                                   const std::string& channelName ) const
{
    return build_User_Command(fd, "JOIN", channelName, "");
}

std::string IrcMessageBuilder::build_Part_Message( int fd,
                                                   const std::string& channelName,
                                                   const std::string& reason ) const
{
    return build_User_Command(fd, "PART", channelName, reason);
}

std::string IrcMessageBuilder::build_Quit_Message( int fd,
                                                   const std::string& reason ) const
{
    return build_User_Command(fd, "QUIT", "", reason);
}

std::string IrcMessageBuilder::build_Nick_Message( int fd,
                                                   const std::string& newNick ) const
{
    return build_User_Command(fd, "NICK", "", newNick);
}

std::string IrcMessageBuilder::build_Privmsg_Message( int fd,
                                                      const std::string& target,
                                                      const std::string& text ) const
{
    return build_User_Command(fd, "PRIVMSG", target, text);
}

std::string IrcMessageBuilder::build_Topic_Message( int fd,
                                                    const std::string& channelName,
                                                    const std::string& topic ) const
{
    return build_User_Command(fd, "TOPIC", channelName, topic);
}

std::string IrcMessageBuilder::build_Invite_Message( int fd,
                                                     const std::string& targetNick,
                                                     const std::string& channelName ) const
{
    return build_User_Command(fd, "INVITE", targetNick, channelName);
}

std::string IrcMessageBuilder::build_Channel_Names( const std::string& channelName ) const
{
    std::vector<int> members;
    _channels.collect_Channel_Members(channelName, members);

    std::string names;

    for (size_t i = 0; i < members.size(); ++i)
    {
        if (!names.empty())
            names += " ";

        if (_channels.is_Operator(channelName, members[i]))
            names += "@";

        names += nick_Or_star(members[i]);
    }

    return names;
}

std::string IrcMessageBuilder::build_Kick_Message( int fd,
                                                   const std::string& channelName,
                                                   const std::string& targetNick,
                                                   const std::string& comment ) const
{
    return build_User_Command(fd, "KICK", channelName + " " + targetNick, comment);
}

std::string IrcMessageBuilder::build_Mode_Message( int fd,
                                                   const std::string& target,
                                                   const std::string& modes,
                                                   const std::string& param ) const
{
    std::string params = target + " " + modes;
    append_Param(params, param);
    return build_User_Command(fd, "MODE", params, "");
}

std::string IrcMessageBuilder::build_Cap_Message( const std::string& target,
                                                  const std::string& subCommand,
                                                  const std::string& text ) const
{
    std::string msg = build_Server_Prefix() + " CAP ";
    msg += normalize_Target(target);
    msg += " ";
    msg += subCommand;
    msg += " :";
    msg += text;
    msg += "\r\n";
    return msg;
}

std::string IrcMessageBuilder::build_Pong_Message( const std::string& token ) const
{
    return build_Server_Prefix() + " PONG :" + token + "\r\n";
}
