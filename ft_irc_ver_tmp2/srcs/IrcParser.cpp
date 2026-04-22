#include "IrcParser.hpp"
#include "IrcCommand.hpp"


#include <iostream> // 디버깅

static void cmd_clear( IrcCommand& cmd )
{
    cmd.raw_Line.clear();
    cmd.prefix.clear();
    cmd.verb.clear();
    cmd.params.clear();
    cmd.trailing.clear();

    cmd.hasTrailing = false;
}

// 일단 혹시 모르니 한번더 확인
void IrcParser::parse_Stripcr( std::string& raw_String )
{
    if (!raw_String.empty() && raw_String[raw_String.size() - 1] == '\r')
    {
        //디버깅
        std::cout << "Strip cr\n";
        raw_String.erase(raw_String.size() - 1);
    }
}

bool IrcParser::parse_Prefix( const std::string& raw_String, size_t& pos, std::string& out_Prefix )
{
    if (pos >= raw_String.size())
        return false;
    if (raw_String[pos] != ':')
        return false; // : 

    // ':' 다음부터 공백 전까지
    size_t sp = raw_String.find(' ', pos);
    if (sp == std::string::npos)
        return false; // prefix만 있고 verb 없음

    // prefix 내용 (':' 제외)
    if (sp == pos + 1)
        return false; // ":" 다음이 바로 공백이면 비정상(빈 prefix)

    out_Prefix = raw_String.substr(pos + 1, sp - (pos + 1));

    // pos 이동: 공백 이후 첫 non-space
    pos = sp + 1;
    while (pos < raw_String.size() && raw_String[pos] == ' ')
        pos++;

    return true;
}

bool IrcParser::parse_Verb( const std::string& raw_String, size_t& pos, std::string& out_Verb )
{
    // 공백 스킵(널널하게)
    while (pos < raw_String.size() && raw_String[pos] == ' ')
        pos++;

    if (pos >= raw_String.size())
        return false;

    // verb는 공백 전까지
    size_t sp = raw_String.find(' ', pos);
    if (sp == std::string::npos)
        sp = raw_String.size();

    out_Verb = raw_String.substr(pos, sp - pos);
    if (out_Verb.empty())
        return false;

    // uppercase normalize (dispatcher 편의)
    for (size_t i = 0; i < out_Verb.size(); ++i) {
        char &ch = out_Verb[i];
        if (ch >= 'a' && ch <= 'z')
            ch = static_cast<char>(ch - 'a' + 'A');
    }

    // pos 이동: verb 뒤 첫 non-space
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

    // trailing 시작 조건:
    // 1) pos 위치가 바로 ':' 인 경우
    // 2) 중간 어딘가에서 " :" 패턴이 나온 경우
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

    // 1) CR 제거
    parse_Stripcr(cmd.raw_Line);

    // 2) leading spaces 스킵
    size_t pos = 0;
    while (pos < cmd.raw_Line.size() && cmd.raw_Line[pos] == ' ')
        pos++;

    if (pos >= cmd.raw_Line.size())
        return false;

    // 3) prefix (optional)
    if (cmd.raw_Line[pos] == ':' && !parse_Prefix(cmd.raw_Line, pos, cmd.prefix))
        return false; // ':'인데 prefix 파싱 실패면 malformed

    // 4) verb (required)
    if (!parse_Verb(cmd.raw_Line, pos, cmd.verb))
        return false;

    parse_Params_Trailing(cmd.raw_Line, pos, cmd.params, cmd.trailing, cmd.hasTrailing);
    return true;
}