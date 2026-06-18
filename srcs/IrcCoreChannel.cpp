#include "IrcCore.hpp"

// JOIN 명령을 처리해 채널 참가, 토픽, NAMES 응답을 보낸다.
void IrcCore::handle_Join( ClientEntry& entry,
                           const IrcCommand& cmd,
                           std::vector<ServerAction>& out )
{
    if (cmd.params.empty())
    {
        reply_Need_More_Params(entry, cmd, out, "JOIN", "[JOIN] channel name missing\n");
        return;
    }

    const std::string& channelName = cmd.params[0];
    const std::string nick = current_Nick(entry.fd);

    if (channelName.empty() || channelName[0] != '#')
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Err_No_such_channel(nick, channelName),
                        "[JOIN] bad channel name\n");
        return;
    }

    std::string providedKey;
    if (cmd.params.size() >= 2)
        providedKey = cmd.params[1];

    const handleResult result = check_Join(channelName, entry.fd, providedKey);

    if (result == JOIN_ALREADY_MEMBER)
    {
        trace_Full(entry, cmd, "[JOIN] already joined\n");
        return;
    }

    if (result == JOIN_INVITE_ONLY)
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Err_Invite_only_chan(nick, channelName),
                        "[JOIN] invite only channel\n");
        return;
    }

    if (result == JOIN_BAD_KEY)
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Err_Bad_channel_key(nick, channelName),
                        "[JOIN] bad key\n");
        return;
    }

    if (result == JOIN_CHANNEL_FULL)
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Err_Channel_is_full(nick, channelName),
                        "[JOIN] channel full\n");
        return;
    }

    if (!_channels.join_Channel(channelName, entry.fd))
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Err_No_such_channel(nick, channelName),
                        "[JOIN] join channel failed\n");
        return;
    }

    const std::string joinMsg = _messages.build_Join_Message(entry.fd, channelName);
    send_To_Channel(channelName, joinMsg, out, -1);

    const std::string topic = _channels.get_Topic(channelName);
    if (topic.empty())
    {
        push_Send(out, entry.fd,
                  _messages.build_Rpl_No_topic(nick, channelName));
    }
    else
    {
        push_Send(out, entry.fd,
                  _messages.build_Rpl_Topic(nick, channelName, topic));
    }

    push_Send(out, entry.fd,
              _messages.build_Rpl_Name_reply(nick, channelName));
    push_Send(out, entry.fd,
              _messages.build_Rpl_End_of_names(nick, channelName));

    trace_Full(entry, cmd, "[JOIN] joined channel\n");
}

// PART 명령을 처리해 채널에서 나가고 멤버들에게 알린다.
void IrcCore::handle_Part( ClientEntry& entry,
                           const IrcCommand& cmd,
                           std::vector<ServerAction>& out )
{
    if (cmd.params.empty() || cmd.params[0].empty())
    {
        reply_Need_More_Params(entry, cmd, out, "PART", "[PART] channel name missing\n");
        return;
    }

    const std::string& channelName = cmd.params[0];

    if (!require_Channel_Exists(entry, cmd, channelName, out, "[PART] no such channel\n"))
        return;

    if (!require_Channel_Member(entry, cmd, channelName, out, "[PART] not a member\n"))
        return;

    const std::string reason = cmd.trailing.empty() ? "" : cmd.trailing;
    const std::string partMsg =
        _messages.build_Part_Message(entry.fd, channelName, reason);

    if (!_channels.remove_Member_And_Cleanup(channelName, entry.fd))
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Err_Not_on_channel(
                            current_Nick(entry.fd),
                            channelName),
                        "[PART] remove member failed\n");
        return;
    }

    send_To_Channel(channelName, partMsg, out, -1);
    push_Send(out, entry.fd, partMsg);

    trace_Full(entry, cmd, "[PART] left channel\n");
}

// PRIVMSG 명령을 처리해 채널 또는 사용자에게 메시지를 전달한다.
void IrcCore::handle_Privmsg( ClientEntry& entry,
                              const IrcCommand& cmd,
                              std::vector<ServerAction>& out )
{
    const std::string senderNick = current_Nick(entry.fd);

    if (cmd.params.empty() || cmd.params[0].empty())
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Err_No_recipient(senderNick, "PRIVMSG"),
                        "[PRIVMSG] recipient missing\n");
        return;
    }

    if (cmd.trailing.empty())
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Err_No_text_to_send(senderNick),
                        "[PRIVMSG] text missing\n");
        return;
    }

    const std::string& target = cmd.params[0];
    const std::string msg =
        _messages.build_Privmsg_Message(entry.fd, target, cmd.trailing);

    if (target[0] == '#')
    {
        if (!_channels.has_Channel(target))
        {
            reply_And_Trace(entry, cmd, out,
                            _messages.build_Err_No_such_channel(senderNick, target),
                            "[PRIVMSG] no such channel\n");
            return;
        }

        if (!_channels.has_Member(target, entry.fd))
        {
            reply_And_Trace(entry, cmd, out,
                            _messages.build_Err_Cannot_send_to_chan(senderNick, target),
                            "[PRIVMSG] sender not in channel\n");
            return;
        }

        send_To_Channel(target, msg, out, entry.fd);
        trace_Full(entry, cmd, "[PRIVMSG] sent to channel\n");
        return;
    }

    ClientEntry* targetClient =
        find_Target_Client(entry, cmd, target, out, "[PRIVMSG] no such nick\n");
    if (targetClient == NULL)
        return;

    push_Send(out, targetClient->fd, msg);
    trace_Full(entry, cmd, "[PRIVMSG] sent to user\n");
}

// TOPIC 명령을 처리해 채널 토픽을 조회하거나 변경한다.
void IrcCore::handle_Topic( ClientEntry& entry,
                            const IrcCommand& cmd,
                            std::vector<ServerAction>& out )
{
    if (cmd.params.empty())
    {
        reply_Need_More_Params(entry, cmd, out, "TOPIC", "[TOPIC] channel name missing\n");
        return;
    }

    const std::string& channelName = cmd.params[0];
    const std::string nick = current_Nick(entry.fd);

    if (!require_Channel_Exists(entry, cmd, channelName, out, "[TOPIC] no such channel\n"))
        return;

    if (!require_Channel_Member(entry, cmd, channelName, out, "[TOPIC] not on channel\n"))
        return;

    if (!cmd.hasTrailing)
    {
        const std::string topic = _channels.get_Topic(channelName);

        if (topic.empty())
        {
            reply_And_Trace(entry, cmd, out,
                            _messages.build_Rpl_No_topic(nick, channelName),
                            "[TOPIC] no topic\n");
            return;
        }

        reply_And_Trace(entry, cmd, out,
                        _messages.build_Rpl_Topic(nick, channelName, topic),
                        "[TOPIC] topic viewed\n");
        return;
    }

    if (!require_Channel_Privilege(
            entry,
            cmd,
            _channels.can_Change_Topic(channelName, entry.fd),
            channelName,
            out,
            "[TOPIC] operator privilege required\n"))
    {
        return;
    }

    if (!_channels.set_Topic(channelName, cmd.trailing))
    {
        trace_Full(entry, cmd, "[TOPIC] set topic failed\n");
        return;
    }

    const std::string topicMsg =
        _messages.build_Topic_Message(entry.fd, channelName, cmd.trailing);

    send_To_Channel(channelName, topicMsg, out, -1);
    trace_Full(entry, cmd, "[TOPIC] topic changed\n");
}

// INVITE 명령을 처리해 대상 사용자를 채널 초대 목록에 추가한다.
void IrcCore::handle_Invite( ClientEntry& entry,
                             const IrcCommand& cmd,
                             std::vector<ServerAction>& out )
{
    if (cmd.params.size() < 2 ||
        cmd.params[0].empty() ||
        cmd.params[1].empty())
    {
        reply_Need_More_Params(entry, cmd, out, "INVITE", "[INVITE] need more params\n");
        return;
    }

    const std::string& targetNick = cmd.params[0];
    const std::string& channelName = cmd.params[1];
    const std::string nick = current_Nick(entry.fd);

    ClientEntry* targetClient =
        find_Target_Client(entry, cmd, targetNick, out, "[INVITE] no such nick\n");
    if (targetClient == NULL)
        return;

    if (!require_Channel_Exists(entry, cmd, channelName, out, "[INVITE] no such channel\n"))
        return;

    if (!require_Channel_Member(
            entry,
            cmd,
            channelName,
            out,
            "[INVITE] requester not on channel\n"))
    {
        return;
    }

    if (!require_Channel_Privilege(
            entry,
            cmd,
            _channels.can_Invite(channelName, entry.fd),
            channelName,
            out,
            "[INVITE] operator privilege required\n"))
    {
        return;
    }

    if (_channels.has_Member(channelName, targetClient->fd))
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Err_User_on_channel(nick, targetNick, channelName),
                        "[INVITE] target already on channel\n");
        return;
    }

    _channels.add_Invite(channelName, targetClient->fd);

    push_Send(out, entry.fd,
              _messages.build_Rpl_Inviting(nick, targetNick, channelName));
    push_Send(out, targetClient->fd,
              _messages.build_Invite_Message(entry.fd, targetNick, channelName));

    trace_Full(entry, cmd, "[INVITE] invite processed\n");
}

// KICK 명령을 처리해 대상 사용자를 채널에서 제거한다.
void IrcCore::handle_Kick( ClientEntry& entry,
                           const IrcCommand& cmd,
                           std::vector<ServerAction>& out )
{
    if (cmd.params.size() < 2 ||
        cmd.params[0].empty() ||
        cmd.params[1].empty())
    {
        reply_Need_More_Params(entry, cmd, out, "KICK", "[KICK] need more params\n");
        return;
    }

    const std::string& channelName = cmd.params[0];
    const std::string& targetNick = cmd.params[1];

    if (!require_Channel_Exists(entry, cmd, channelName, out, "[KICK] no such channel\n"))
        return;

    if (!require_Channel_Member(
            entry,
            cmd,
            channelName,
            out,
            "[KICK] requester not on channel\n"))
    {
        return;
    }

    if (!require_Channel_Privilege(
            entry,
            cmd,
            _channels.can_Kick(channelName, entry.fd),
            channelName,
            out,
            "[KICK] operator privilege required\n"))
    {
        return;
    }

    ClientEntry* targetClient =
        find_Target_Client(entry, cmd, targetNick, out, "[KICK] no such nick\n");
    if (targetClient == NULL)
        return;

    if (!require_Target_Channel_Member(
            entry,
            cmd,
            channelName,
            targetNick,
            targetClient->fd,
            out,
            "[KICK] target not on channel\n"))
    {
        return;
    }

    const std::string comment = cmd.trailing.empty() ? targetNick : cmd.trailing;
    const std::string kickMsg =
        _messages.build_Kick_Message(entry.fd, channelName, targetNick, comment);

    if (!_channels.remove_Member_And_Cleanup(channelName, targetClient->fd))
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Err_User_not_in_channel(
                            current_Nick(entry.fd),
                            targetNick,
                            channelName),
                        "[KICK] remove member failed\n");
        return;
    }

    send_To_Channel(channelName, kickMsg, out, entry.fd);
    push_Send(out, entry.fd, kickMsg);
    if (targetClient->fd != entry.fd)
        push_Send(out, targetClient->fd, kickMsg);

    trace_Full(entry, cmd, "[KICK] kick processed\n");
}

// 사용자 MODE 명령을 조회하거나 적용한다.
void IrcCore::handle_User_Mode( ClientEntry& entry,
                                const IrcCommand& cmd,
                                std::vector<ServerAction>& out )
{
    const std::string& targetNick = cmd.params[0];
    const std::string nick = current_Nick(entry.fd);

    ClientEntry* targetClient = _clients.find_By_Nick(targetNick);
    if (targetClient == NULL || targetClient->fd != entry.fd)
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Err_Users_dont_match(nick),
                        "[MODE] cannot inspect or change another user's mode\n");
        return;
    }

    if (cmd.params.size() < 2 || cmd.params[1].empty())
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Rpl_User_mode_is(
                            nick,
                            _clients.get_User_Modes(entry.fd)),
                        "[MODE] user mode viewed\n");
        return;
    }

    const std::string& modeString = cmd.params[1];
    char currentSign = 0;
    bool queryOnly = true;

    for (size_t i = 0; i < modeString.size(); ++i)
    {
        const char modeChar = modeString[i];

        if (modeChar == '+' || modeChar == '-')
        {
            currentSign = modeChar;
            continue;
        }

        queryOnly = false;

        if (currentSign == 0)
        {
            reply_And_Trace(entry, cmd, out,
                            _messages.build_Err_Umode_unknown_flag(nick),
                            "[MODE] sign missing before user mode\n");
            return;
        }

        if (!is_Supported_User_Mode(modeChar))
        {
            reply_And_Trace(entry, cmd, out,
                            _messages.build_Err_Umode_unknown_flag(nick),
                            "[MODE] unsupported user mode\n");
            return;
        }
    }

    if (queryOnly)
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Rpl_User_mode_is(
                            nick,
                            _clients.get_User_Modes(entry.fd)),
                        "[MODE] user mode viewed\n");
        return;
    }

    std::string appliedModes;
    std::string appliedParams;
    currentSign = 0;

    for (size_t i = 0; i < modeString.size(); ++i)
    {
        const char modeChar = modeString[i];

        if (modeChar == '+' || modeChar == '-')
        {
            currentSign = modeChar;
            continue;
        }

        if (!apply_User_Mode(entry.fd, currentSign, modeChar))
        {
            reply_And_Trace(entry, cmd, out,
                            _messages.build_Err_Umode_unknown_flag(nick),
                            "[MODE] failed to apply user mode\n");
            return;
        }

        append_Mode_Change(appliedModes, appliedParams, currentSign, modeChar, "");
    }

    if (appliedModes.empty())
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Rpl_User_mode_is(
                            nick,
                            _clients.get_User_Modes(entry.fd)),
                        "[MODE] no applied user mode\n");
        return;
    }

    push_Send(out, entry.fd,
              _messages.build_Mode_Message(entry.fd, targetNick, appliedModes, ""));

    trace_Full(entry, cmd, "[MODE] user mode changed\n");
}

// MODE 명령을 처리해 사용자 모드 또는 채널 모드를 조회/변경한다.
void IrcCore::handle_Mode( ClientEntry& entry,
                           const IrcCommand& cmd,
                           std::vector<ServerAction>& out )
{
    if (cmd.params.empty())
    {
        reply_Need_More_Params(entry, cmd, out, "MODE", "[MODE] channel name missing\n");
        return;
    }

    const std::string& channelName = cmd.params[0];
    const std::string nick = current_Nick(entry.fd);

    if (channelName.empty() || channelName[0] != '#')
    {
        handle_User_Mode(entry, cmd, out);
        return;
    }

    if (!require_Channel_Exists(entry, cmd, channelName, out, "[MODE] no such channel\n"))
        return;

    if (!require_Channel_Member(
            entry,
            cmd,
            channelName,
            out,
            "[MODE] requester not on channel\n"))
    {
        return;
    }

    if (cmd.params.size() < 2)
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Rpl_Channel_mode_is(nick, channelName),
                        "[MODE] channel mode viewed\n");
        return;
    }

    if (!require_Channel_Privilege(
            entry,
            cmd,
            _channels.can_Change_Mode(channelName, entry.fd),
            channelName,
            out,
            "[MODE] operator privilege required\n"))
    {
        return;
    }

    const std::string& modeString = cmd.params[1];
    char currentSign = 0;
    size_t paramIndex = 2;
    std::string appliedModes;
    std::string appliedParams;

    for (size_t i = 0; i < modeString.size(); ++i)
    {
        const char modeChar = modeString[i];

        if (modeChar == '+' || modeChar == '-')
        {
            currentSign = modeChar;
            continue;
        }

        if (currentSign == 0)
        {
            send_Applied_Channel_Mode(
                entry.fd, channelName, appliedModes, appliedParams, out);
            reply_And_Trace(entry, cmd, out,
                            _messages.build_Err_Unknown_mode(nick, modeChar),
                            "[MODE] sign missing before mode char\n");
            return;
        }

        if (!is_Supported_Channel_Mode(modeChar))
        {
            send_Applied_Channel_Mode(
                entry.fd, channelName, appliedModes, appliedParams, out);
            reply_And_Trace(entry, cmd, out,
                            _messages.build_Err_Unknown_mode(nick, modeChar),
                            "[MODE] unsupported mode char\n");
            return;
        }

        std::string modeParam;
        if (mode_Requires_Param(currentSign, modeChar))
        {
            if (paramIndex >= cmd.params.size())
            {
                send_Applied_Channel_Mode(
                    entry.fd, channelName, appliedModes, appliedParams, out);
                reply_Need_More_Params(entry, cmd, out, "MODE", "[MODE] parameter missing\n");
                return;
            }

            modeParam = cmd.params[paramIndex];
            ++paramIndex;
        }

        std::string appliedParam;
        const handleResult modeResult =
            apply_Channel_Mode(channelName, currentSign, modeChar, modeParam, appliedParam);

        if (modeResult != MODE_APPLY_OK)
        {
            send_Applied_Channel_Mode(
                entry.fd, channelName, appliedModes, appliedParams, out);
            reply_Mode_Error(entry, cmd, out, channelName, modeChar, modeParam, modeResult);
            return;
        }

        append_Mode_Change(appliedModes, appliedParams,
                           currentSign, modeChar, appliedParam);
    }

    if (appliedModes.empty())
    {
        reply_And_Trace(entry, cmd, out,
                        _messages.build_Rpl_Channel_mode_is(nick, channelName),
                        "[MODE] no applied mode\n");
        return;
    }

    send_Applied_Channel_Mode(
        entry.fd, channelName, appliedModes, appliedParams, out);
    trace_Full(entry, cmd, "[MODE] mode changed\n");
}

// QUIT 명령을 처리해 종료 이유를 전파하고 연결을 닫는다.
void IrcCore::handle_Quit( ClientEntry& entry,
                           const IrcCommand& cmd,
                           std::vector<ServerAction>& out )
{
    std::string reason = "Client Quit";
    if (!cmd.trailing.empty())
        reason = cmd.trailing;

    disconnect_Client(entry.fd, reason, out);
    trace_Full(entry, cmd, "[QUIT] quit processed\n");
}
