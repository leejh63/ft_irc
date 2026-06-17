#include "IrcParser.hpp"
#include "IrcCommand.hpp"


static void cmd_clear( IrcCommand& cmd )
{
    cmd.raw_Line.clear();
    cmd.prefix.clear();
    cmd.verb.clear();
    cmd.params.clear();
    cmd.trailing.clear();

    cmd.hasTrailing = false;
}

void IrcParser::parse_Stripcr( std::string& raw_String )
{
    if (!raw_String.empty() && raw_String[raw_String.size() - 1] == '\r')
        raw_String.erase(raw_String.size() - 1);
}

bool IrcParser::parse_Prefix( const std::string& raw_String, size_t& pos, std::string& out_Prefix )
{
    if (pos >= raw_String.size())
        return false;
    if (raw_String[pos] != ':')
        return false;

    size_t sp = raw_String.find(' ', pos);
    if (sp == std::string::npos)
        return false;

    if (sp == pos + 1)
        return false;

    out_Prefix = raw_String.substr(pos + 1, sp - (pos + 1));

    pos = sp + 1;
    while (pos < raw_String.size() && raw_String[pos] == ' ')
        pos++;

    return true;
}

bool IrcParser::parse_Verb( const std::string& raw_String, size_t& pos, std::string& out_Verb )
{
    while (pos < raw_String.size() && raw_String[pos] == ' ')
        pos++;

    if (pos >= raw_String.size())
        return false;

    size_t sp = raw_String.find(' ', pos);
    if (sp == std::string::npos)
        sp = raw_String.size();

    out_Verb = raw_String.substr(pos, sp - pos);
    if (out_Verb.empty())
        return false;

    // Normalize command verb for dispatch.
    for (size_t i = 0; i < out_Verb.size(); ++i) {
        char &ch = out_Verb[i];
        if (ch >= 'a' && ch <= 'z')
            ch = static_cast<char>(ch - 'a' + 'A');
    }

    pos = sp;
    while (pos < raw_String.size() && raw_String[pos] == ' ')
        pos++;

    return true;
}

void IrcParser::parse_Params_Trailing( const std::string& raw_String,
                                       size_t pos,
                                       std::vector<std::string>& out_Params,
                                       std::string& out_Trailing,
                                       bool& out_HasTrailing )
{
    size_t      t;
    std::string raw_Params;
    size_t      i;
    size_t      j;

    out_HasTrailing = false;

    // Detect the IRC trailing parameter.
    if (pos < raw_String.size() && raw_String[pos] == ':')
        t = pos;
    else
        t = raw_String.find(" :", pos);

    if (t != std::string::npos)
    {
        out_HasTrailing = true;

        if (t == pos)
            raw_Params.clear();
        else
            raw_Params = raw_String.substr(pos, t - pos);

        if (raw_String[t] == ':')
            out_Trailing = raw_String.substr(t + 1);
        else
            out_Trailing = raw_String.substr(t + 2);
    }
    else
    {
        raw_Params = raw_String.substr(pos);
    }

    i = 0;
    while (i < raw_Params.size())
    {
        while (i < raw_Params.size() && raw_Params[i] == ' ')
            i++;
        if (i >= raw_Params.size())
            break;

        j = raw_Params.find(' ', i);
        if (j == std::string::npos)
            j = raw_Params.size();

        out_Params.push_back(raw_Params.substr(i, j - i));
        i = j;
    }
}

bool IrcParser::parse_Line( const std::string& raw_Line, IrcCommand& cmd )
{
    cmd_clear(cmd);
    cmd.raw_Line = raw_Line;

    parse_Stripcr(cmd.raw_Line);

    size_t pos = 0;
    while (pos < cmd.raw_Line.size() && cmd.raw_Line[pos] == ' ')
        pos++;

    if (pos >= cmd.raw_Line.size())
        return false;

    // 3) prefix (optional)
    if (cmd.raw_Line[pos] == ':' && !parse_Prefix(cmd.raw_Line, pos, cmd.prefix))
        return false;

    // 4) verb (required)
    if (!parse_Verb(cmd.raw_Line, pos, cmd.verb))
        return false;

    parse_Params_Trailing(cmd.raw_Line, pos, cmd.params, cmd.trailing, cmd.hasTrailing);
    return true;
}