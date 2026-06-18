#include "ClientRegistry.hpp"

#include <algorithm>

namespace
{
    // 클라이언트의 등록 관련 상태를 초기값으로 되돌린다.
    void reset_Client_State( ClientEntry& entry )
    {
        entry.passOk = false;
        entry.hasNick = false;
        entry.hasUser = false;
        entry.registered = false;
        entry.nick.clear();
        entry.user.clear();
        entry.realName.clear();
        entry.userModes.clear();
    }

    // 새 클라이언트 엔트리를 기본 상태로 생성한다.
    ClientEntry make_Client_Entry( int fd )
    {
        ClientEntry entry;

        entry.fd = fd;
        entry.inBuf.clear();
        entry.outBuf.clear();
        reset_Client_State(entry);

        return entry;
    }

    // IRC 닉네임 비교 규칙에 맞춰 한 문자를 접는다.
    char fold_Nick_Char( char c )
    {
        if (c >= 'A' && c <= 'Z')
            return static_cast<char>(c - 'A' + 'a');
        if (c == '[')
            return '{';
        if (c == ']')
            return '}';
        if (c == '\\')
            return '|';
        if (c == '~')
            return '^';
        return c;
    }

    // IRC 닉네임 비교 규칙에 맞춰 닉네임 전체를 정규화한다.
    std::string fold_Nick( const std::string& nick )
    {
        std::string folded = nick;

        for (size_t i = 0; i < folded.size(); ++i)
            folded[i] = fold_Nick_Char(folded[i]);

        return folded;
    }

    // 클라이언트 엔트리의 닉네임이 주어진 닉네임과 같은지 비교한다.
    bool matches_Nick( const ClientEntry& entry, const std::string& nick )
    {
        return entry.hasNick && fold_Nick(entry.nick) == fold_Nick(nick);
    }

    // PASS, NICK, USER가 모두 준비되어 등록 가능한지 확인한다.
    bool can_Register( const ClientEntry& entry )
    {
        return !entry.registered
            && entry.passOk
            && entry.hasNick
            && entry.hasUser;
    }

    // 사용자 모드 문자열에 모드 플래그를 정렬된 위치로 추가한다.
    void add_Mode_Flag( std::string& modes, char mode )
    {
        if (modes.find(mode) != std::string::npos)
            return;

        std::string::iterator pos = modes.begin();
        while (pos != modes.end() && *pos < mode)
            ++pos;

        modes.insert(pos, mode);
    }

    // 사용자 모드 문자열에서 지정한 모드 플래그를 제거한다.
    void remove_Mode_Flag( std::string& modes, char mode )
    {
        modes.erase(std::remove(modes.begin(), modes.end(), mode), modes.end());
    }
}

// 비어 있는 클라이언트 레지스트리를 생성한다.
ClientRegistry::ClientRegistry( void )
: _clients()
{
}

// 클라이언트 레지스트리 자원을 정리한다.
ClientRegistry::~ClientRegistry( void )
{
}

// 해당 파일 디스크립터의 클라이언트가 등록되어 있는지 확인한다.
bool ClientRegistry::has_Client( int fd ) const
{
    return (_clients.find(fd) != _clients.end());
}

// 파일 디스크립터로 수정 가능한 클라이언트 엔트리를 찾는다.
ClientEntry* ClientRegistry::find_By_Fd( int fd )
{
    std::map<int, ClientEntry>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return NULL;
    return &(it->second);
}

// 파일 디스크립터로 읽기 전용 클라이언트 엔트리를 찾는다.
const ClientEntry* ClientRegistry::find_By_Fd( int fd ) const
{
    std::map<int, ClientEntry>::const_iterator it = _clients.find(fd);
    if (it == _clients.end())
        return NULL;
    return &(it->second);
}

// 닉네임으로 수정 가능한 클라이언트 엔트리를 찾는다.
ClientEntry* ClientRegistry::find_By_Nick( const std::string& nick )
{
    std::map<int, ClientEntry>::iterator it = _clients.begin();
    std::map<int, ClientEntry>::iterator end = _clients.end();

    while (it != end)
    {
        if (matches_Nick(it->second, nick))
            return &(it->second);
        ++it;
    }
    return NULL;
}

// 닉네임으로 읽기 전용 클라이언트 엔트리를 찾는다.
const ClientEntry* ClientRegistry::find_By_Nick( const std::string& nick ) const
{
    std::map<int, ClientEntry>::const_iterator it = _clients.begin();
    std::map<int, ClientEntry>::const_iterator end = _clients.end();

    while (it != end)
    {
        if (matches_Nick(it->second, nick))
            return &(it->second);
        ++it;
    }
    return NULL;
}

// 새 클라이언트 엔트리를 추가한다.
bool ClientRegistry::add_Client( int fd )
{
    if (has_Client(fd))
        return false;

    _clients.insert(std::make_pair(fd, make_Client_Entry(fd)));
    return true;
}

// 파일 디스크립터에 해당하는 클라이언트 엔트리를 제거한다.
bool ClientRegistry::remove_Client( int fd )
{
    std::map<int, ClientEntry>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return false;

    _clients.erase(it);
    return true;
}

// 지정한 클라이언트를 제외하고 닉네임이 이미 사용 중인지 확인한다.
bool ClientRegistry::is_Nick_In_Use( const std::string& nick,
                                     int exceptFd ) const
{
    std::map<int, ClientEntry>::const_iterator it = _clients.begin();
    std::map<int, ClientEntry>::const_iterator end = _clients.end();

    while (it != end)
    {
        if (it->first != exceptFd && matches_Nick(it->second, nick))
            return true;
        ++it;
    }
    return false;
}

// 클라이언트가 IRC 등록 절차를 마쳤는지 확인한다.
bool ClientRegistry::is_Registered( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;

    return entry->registered;
}

// 등록 조건을 만족하는 클라이언트를 등록 완료 상태로 바꾼다.
bool ClientRegistry::try_Register( int fd )
{
    ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;

    if (!can_Register(*entry))
        return false;

    entry->registered = true;
    return true;
}

// PASS 검증 통과 여부를 저장한다.
bool ClientRegistry::set_Pass_Ok( int fd, bool value )
{
    ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;

    entry->passOk = value;
    return true;
}

// 클라이언트 닉네임을 저장하고 보유 여부를 갱신한다.
bool ClientRegistry::set_Nick( int fd, const std::string& newNick )
{
    ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;

    entry->nick = newNick;
    entry->hasNick = !newNick.empty();
    return true;
}

// USER 명령으로 받은 사용자명과 실명을 저장한다.
bool ClientRegistry::set_User( int fd,
                               const std::string& user,
                               const std::string& realName )
{
    ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;

    entry->user = user;
    entry->realName = realName;
    entry->hasUser = !user.empty();
    return true;
}

// PASS 검증을 통과했는지 확인한다.
bool ClientRegistry::has_Pass_Ok( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;
    return entry->passOk;
}

// 닉네임이 설정되어 있는지 확인한다.
bool ClientRegistry::has_Nick( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;
    return entry->hasNick;
}

// USER 정보가 설정되어 있는지 확인한다.
bool ClientRegistry::has_User( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;
    return entry->hasUser;
}

// 사용자 모드를 켜거나 끈다.
bool ClientRegistry::set_User_Mode( int fd, char mode, bool enabled )
{
    ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;

    if (enabled)
        add_Mode_Flag(entry->userModes, mode);
    else
        remove_Mode_Flag(entry->userModes, mode);

    return true;
}

// 현재 사용자 모드 문자열을 반환한다.
std::string ClientRegistry::get_User_Modes( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return "";
    return entry->userModes;
}

// 현재 닉네임을 반환한다.
std::string ClientRegistry::get_Nick( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return "";
    return entry->nick;
}

// 현재 사용자명을 반환한다.
std::string ClientRegistry::get_User( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return "";
    return entry->user;
}

// 현재 실명을 반환한다.
std::string ClientRegistry::get_Real_Name( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return "";
    return entry->realName;
}
