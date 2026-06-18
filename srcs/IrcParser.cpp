#include "IrcParser.hpp"
#include "IrcCommand.hpp"


// 재사용할 IrcCommand 구조체를 빈 상태로 초기화한다.
static void cmd_clear( IrcCommand& cmd )
{
    cmd.raw_Line.clear();
    cmd.prefix.clear();
    cmd.verb.clear();
    cmd.params.clear();
    cmd.trailing.clear();

    cmd.hasTrailing = false;
}

// IRC 라인 끝에 남아 있는 CR 문자를 제거한다.
void IrcParser::parse_Stripcr( std::string& raw_String )
{
    if (!raw_String.empty() && raw_String[raw_String.size() - 1] == '\r')
        raw_String.erase(raw_String.size() - 1);
}

// 선택적인 접두부를 파싱하고 다음 위치로 이동한다.
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

// 명령어 이름을 파싱하고 대문자로 정규화한다.
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

    // 명령 디스패치를 위해 명령어 이름을 대문자로 맞춘다.
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

// 일반 파라미터와 마지막 파라미터를 분리해 저장한다.
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

    // IRC 마지막 파라미터가 시작되는 위치를 찾는다.
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

// 원본 IRC 라인 하나를 접두부, 명령어, 파라미터로 파싱한다.
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

    // 접두부는 선택 항목이다.
    if (cmd.raw_Line[pos] == ':' && !parse_Prefix(cmd.raw_Line, pos, cmd.prefix))
        return false;

    // 명령어 이름은 필수 항목이다.
    if (!parse_Verb(cmd.raw_Line, pos, cmd.verb))
        return false;

    parse_Params_Trailing(cmd.raw_Line, pos, cmd.params, cmd.trailing, cmd.hasTrailing);
    return true;
}
