#include "IrcCore.hpp"

void IrcCore::handle_Cap( ClientEntry& entry,
                          const IrcCommand& cmd,
                          std::vector<ServerAction>& out )
{
    const std::string target = _messages.nick_Or_star(entry.fd);

    if (cmd.params.empty())
    {
        debug_Full(entry, cmd, "[CAP] subcommand missing\n");
        return;
    }

    const std::string& subCommand = cmd.params[0];

    if (subCommand == "LS" || subCommand == "LIST")
    {
        reply_And_Debug(entry, cmd, out,
                        _messages.build_Cap_Message(target, subCommand, ""),
                        "[CAP] empty capability list sent\n");
        return;
    }

    if (subCommand == "REQ")
    {
        reply_And_Debug(entry, cmd, out,
                        _messages.build_Cap_Message(target, "NAK", cmd.trailing),
                        "[CAP] capability request rejected\n");
        return;
    }

    if (subCommand == "END")
    {
        debug_Full(entry, cmd, "[CAP] capability negotiation ended\n");
        return;
    }

    debug_Full(entry, cmd, "[CAP] unsupported subcommand ignored\n");
}

void IrcCore::handle_Ping( ClientEntry& entry,
                           const IrcCommand& cmd,
                           std::vector<ServerAction>& out )
{
    std::string token;

    if (!cmd.trailing.empty())
        token = cmd.trailing;
    else if (!cmd.params.empty())
        token = cmd.params[0];

    if (token.empty())
    {
        reply_Need_More_Params(entry, cmd, out, "PING", "[PING] token missing\n");
        return;
    }

    reply_And_Debug(entry, cmd, out,
                    _messages.build_Pong_Message(token),
                    "[PING] pong sent\n");
}

void IrcCore::handle_Who( ClientEntry& entry,
                          const IrcCommand& cmd,
                          std::vector<ServerAction>& out )
{
    const std::string requesterNick = _messages.nick_Or_star(entry.fd);

    if (cmd.params.empty())
    {
        reply_And_Debug(entry, cmd, out,
                        _messages.build_Rpl_End_of_who(requesterNick, "*"),
                        "[WHO] no mask, end of who\n");
        return;
    }

    const std::string& mask = cmd.params[0];

    if (mask.empty() || mask[0] != '#')
    {
        reply_And_Debug(entry, cmd, out,
                        _messages.build_Rpl_End_of_who(requesterNick, mask.empty() ? "*" : mask),
                        "[WHO] unsupported non-channel who\n");
        return;
    }

    if (!_channels.has_Channel(mask))
    {
        reply_And_Debug(entry, cmd, out,
                        _messages.build_Rpl_End_of_who(requesterNick, mask),
                        "[WHO] no such channel, end of who\n");
        return;
    }

    std::vector<int> members;
    _channels.collect_Channel_Members(mask, members);

    for (size_t i = 0; i < members.size(); ++i)
    {
        const std::string reply =
            _messages.build_Rpl_Who_reply(requesterNick, mask, members[i]);

        if (!reply.empty())
            push_Send(out, entry.fd, reply);
    }

    push_Send(out, entry.fd,
              _messages.build_Rpl_End_of_who(requesterNick, mask));

    debug_Full(entry, cmd, "[WHO] channel who replied\n");
}

void IrcCore::handle_Pong( ClientEntry& entry,
                           const IrcCommand& cmd,
                           std::vector<ServerAction>& out )
{
    (void)out;
    debug_Full(entry, cmd, "[PONG] pong received\n");
}

void IrcCore::handle_Error( ClientEntry& entry,
                            const IrcCommand& cmd,
                            std::vector<ServerAction>& out )
{
    (void)out;
    debug_Full(entry, cmd, debug_Message(HANDLE_ERROR));
}

void IrcCore::handle_Unknown( ClientEntry& entry,
                              const IrcCommand& cmd,
                              std::vector<ServerAction>& out )
{
    reply_And_Debug(entry, cmd, out,
                    _messages.build_Err_Unknown_command(current_Nick(entry.fd), cmd.verb),
                    debug_Message(HANDLE_UNKNOWN));
}
