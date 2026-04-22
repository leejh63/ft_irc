#include "IrcCore.hpp"

void IrcCore::handle_Pass( ClientEntry& entry,
                           const IrcCommand& cmd,
                           std::vector<ServerAction>& out )
{
    const handleResult result = check_Pass(entry.fd, cmd);

    if (result == PASS_PASSWORD_OK)
    {
        _clients.set_Pass_Ok(entry.fd, true);
        try_Register(entry, out);
        debug_Full(entry, cmd, debug_Message(result));
        return;
    }

    if (result == PASS_PASSWORD_BAD)
    {
        _clients.set_Pass_Ok(entry.fd, false);
        reply_And_Debug(entry, cmd, out,
                        _messages.build_Err_Password_mismatch(current_Nick(entry.fd)),
                        debug_Message(result));
        return;
    }

    if (result == PASS_PARAM_MISSING)
    {
        reply_Need_More_Params(entry, cmd, out, "PASS", debug_Message(result));
        return;
    }

    reply_And_Debug(entry, cmd, out,
                    _messages.build_Err_Already_registered(current_Nick(entry.fd)),
                    debug_Message(result));
}

void IrcCore::handle_Nick( ClientEntry& entry,
                           const IrcCommand& cmd,
                           std::vector<ServerAction>& out )
{
    if (!ensure_Pass_Accepted(entry, cmd, out, "[NICK] password required first\n"))
        return;

    const handleResult result = check_Nick(cmd);
    const std::string currentNick = current_Nick(entry.fd);

    if (result == NICK_PARAM_MISSING)
    {
        reply_Need_More_Params(entry, cmd, out, "NICK", debug_Message(result));
        return;
    }

    if (result == NICK_ERRONEUS)
    {
        reply_And_Debug(
            entry,
            cmd,
            out,
            _messages.build_Err_Erroneus_nickname(
                currentNick,
                cmd.params.empty() ? "*" : cmd.params[0]),
            debug_Message(result));
        return;
    }

    const std::string& newNick = cmd.params[0];
    const bool hadNick = _clients.has_Nick(entry.fd);

    if (!is_Nick_available(entry.fd, newNick))
    {
        reply_And_Debug(entry, cmd, out,
                        _messages.build_Err_Nickname_in_use(currentNick, newNick),
                        debug_Message(NICK_IN_USE));
        return;
    }

    std::string nickMsg;
    if (hadNick)
        nickMsg = _messages.build_Nick_Message(entry.fd, newNick);

    _clients.set_Nick(entry.fd, newNick);
    try_Register(entry, out);

    if (hadNick)
    {
        push_Send(out, entry.fd, nickMsg);
        send_To_Shared_Peers(entry.fd, nickMsg, out);
        debug_Full(entry, cmd, "[NICK] nickname changed\n");
        return;
    }

    debug_Full(entry, cmd, "[NICK] nickname set\n");
}

void IrcCore::handle_User( ClientEntry& entry,
                           const IrcCommand& cmd,
                           std::vector<ServerAction>& out )
{
    if (!ensure_Pass_Accepted(entry, cmd, out, "[USER] password required first\n"))
        return;

    const handleResult result = check_User(entry.fd, cmd);
    const std::string currentNick = current_Nick(entry.fd);

    if (result == USER_OK)
    {
        _clients.set_User(entry.fd, cmd.params[0], cmd.trailing);
        try_Register(entry, out);
        debug_Full(entry, cmd, debug_Message(result));
        return;
    }

    if (result == USER_PARAM_MISSING || result == USER_REALNAME_MISSING)
    {
        reply_Need_More_Params(entry, cmd, out, "USER", debug_Message(result));
        return;
    }

    reply_And_Debug(entry, cmd, out,
                    _messages.build_Err_Already_registered(currentNick),
                    debug_Message(result));
}
