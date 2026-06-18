#include "ChannelRegistry.hpp"

namespace
{
    // 새 채널 엔트리를 기본 모드와 빈 멤버 목록으로 생성한다.
    ChannelEntry make_Channel_Entry( const std::string& name )
    {
        ChannelEntry entry;

        entry.name = name;
        entry.topic = "";
        entry.inviteOnly = false;
        entry.topicOpOnly = false;
        entry.hasKey = false;
        entry.key = "";
        entry.hasLimit = false;
        entry.userLimit = 0;

        return entry;
    }

    // 채널에 지정한 클라이언트가 참여 중인지 확인한다.
    bool is_Channel_Member( const ChannelEntry& channel, int fd )
    {
        return channel.members.find(fd) != channel.members.end();
    }

    // 채널에서 지정한 클라이언트가 오퍼레이터인지 확인한다.
    bool is_Channel_Operator( const ChannelEntry& channel, int fd )
    {
        return channel.operators.find(fd) != channel.operators.end();
    }

    // 클라이언트가 채널 멤버이면서 오퍼레이터 권한을 갖는지 확인한다.
    bool has_Operator_Privilege( const ChannelEntry& channel, int fd )
    {
        return is_Channel_Member(channel, fd) && is_Channel_Operator(channel, fd);
    }

    // 오퍼레이터가 없는 채널에 남은 멤버 중 한 명을 오퍼레이터로 지정한다.
    void ensure_Channel_Has_Operator( ChannelEntry& channel )
    {
        if (channel.members.empty())
            return;

        if (!channel.operators.empty())
            return;

        channel.operators.insert(*channel.members.begin());
    }

    // 채널에서 클라이언트의 멤버십, 권한, 초대 상태를 모두 제거한다.
    void erase_Client_From_Channel( ChannelEntry& channel, int fd )
    {
        channel.members.erase(fd);
        channel.operators.erase(fd);
        channel.invited.erase(fd);
        ensure_Channel_Has_Operator(channel);
    }
}

// 비어 있는 채널 레지스트리를 생성한다.
ChannelRegistry::ChannelRegistry( void )
{
}

// 채널 레지스트리 자원을 정리한다.
ChannelRegistry::~ChannelRegistry( void )
{
}

// 같은 채널을 공유하는 다른 클라이언트들을 수집한다.
void ChannelRegistry::collect_Shared_Peers( int fd, std::set<int>& outPeers ) const
{
    outPeers.clear();

    std::map<std::string, ChannelEntry>::const_iterator it = _channels.begin();
    for (; it != _channels.end(); ++it)
    {
        const ChannelEntry& channel = it->second;

        if (channel.members.find(fd) == channel.members.end())
            continue;

        std::set<int>::const_iterator mit = channel.members.begin();
        for (; mit != channel.members.end(); ++mit)
            outPeers.insert(*mit);
    }

    outPeers.erase(fd);
}

// 클라이언트가 참여 중인 채널 이름을 수집한다.
void ChannelRegistry::collect_User_Channels( int fd, std::vector<std::string>& outChannels ) const
{
    outChannels.clear();

    std::map<std::string, ChannelEntry>::const_iterator it = _channels.begin();
    for (; it != _channels.end(); ++it)
    {
        if (it->second.members.find(fd) != it->second.members.end())
            outChannels.push_back(it->first);
    }
}

// 이름에 해당하는 채널이 존재하는지 확인한다.
bool ChannelRegistry::has_Channel( const std::string& name ) const
{
    return _channels.find(name) != _channels.end();
}

// 이름으로 수정 가능한 채널 엔트리를 찾는다.
ChannelEntry* ChannelRegistry::find_By_Name( const std::string& name )
{
    std::map<std::string, ChannelEntry>::iterator it = _channels.find(name);
    if (it == _channels.end())
        return NULL;
    return &it->second;
}

// 이름으로 읽기 전용 채널 엔트리를 찾는다.
const ChannelEntry* ChannelRegistry::find_By_Name( const std::string& name ) const
{
    std::map<std::string, ChannelEntry>::const_iterator it = _channels.find(name);
    if (it == _channels.end())
        return NULL;
    return &it->second;
}

// 새 채널을 레지스트리에 추가한다.
bool ChannelRegistry::add_Channel( const std::string& name )
{
    if (has_Channel(name))
        return false;

    _channels.insert(std::make_pair(name, make_Channel_Entry(name)));
    return true;
}

// 이름에 해당하는 채널을 제거한다.
bool ChannelRegistry::remove_Channel( const std::string& name )
{
    return _channels.erase(name) > 0;
}

// 멤버가 없는 채널이면 제거한다.
bool ChannelRegistry::remove_Channel_If_Empty( const std::string& name )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    if (!ch->members.empty())
        return false;

    return remove_Channel(name);
}

// 지정한 클라이언트가 채널 멤버인지 확인한다.
bool ChannelRegistry::has_Member( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    return is_Channel_Member(*ch, fd);
}

// 지정한 클라이언트를 채널 멤버로 추가한다.
bool ChannelRegistry::add_Member( const std::string& name, int fd )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    std::pair<std::set<int>::iterator, bool> result = ch->members.insert(fd);
    return result.second;
}

// 지정한 클라이언트를 채널 멤버에서 제거한다.
bool ChannelRegistry::remove_Member( const std::string& name, int fd )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    if (!is_Channel_Member(*ch, fd))
        return false;

    erase_Client_From_Channel(*ch, fd);
    ensure_Channel_Has_Operator(*ch);
    return true;
}

// 멤버를 제거한 뒤 비어 있는 채널까지 정리한다.
bool ChannelRegistry::remove_Member_And_Cleanup( const std::string& name, int fd )
{
    if (!remove_Member(name, fd))
        return false;

    remove_Channel_If_Empty(name);
    return true;
}

// 채널의 현재 멤버 수를 반환한다.
size_t ChannelRegistry::member_Count( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return 0;

    return ch->members.size();
}

// 지정한 클라이언트가 채널 오퍼레이터인지 확인한다.
bool ChannelRegistry::is_Operator( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    return is_Channel_Operator(*ch, fd);
}

// 지정한 클라이언트를 채널 오퍼레이터로 추가한다.
bool ChannelRegistry::add_Operator( const std::string& name, int fd )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    if (!is_Channel_Member(*ch, fd))
        return false;

    std::pair<std::set<int>::iterator, bool> result = ch->operators.insert(fd);
    return result.second;
}

// 지정한 클라이언트의 채널 오퍼레이터 권한을 제거한다.
bool ChannelRegistry::remove_Operator( const std::string& name, int fd )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    return ch->operators.erase(fd) > 0;
}

// 해당 오퍼레이터를 제거하면 마지막 오퍼레이터가 사라지는지 확인한다.
bool ChannelRegistry::would_Remove_Last_Operator( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    if (!is_Channel_Operator(*ch, fd))
        return false;

    return ch->operators.size() <= 1;
}

// 채널이 초대 전용 모드인지 확인한다.
bool ChannelRegistry::is_Invite_Only( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return ch->inviteOnly;
}

// 채널 토픽 변경이 오퍼레이터에게만 허용되는지 확인한다.
bool ChannelRegistry::is_Topic_Op_Only( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return ch->topicOpOnly;
}

// 채널의 초대 전용 모드를 설정한다.
bool ChannelRegistry::set_Invite_Only( const std::string& name, bool on )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->inviteOnly = on;
    return true;
}

// 채널의 토픽 변경 권한 모드를 설정한다.
bool ChannelRegistry::set_Topic_Op_Only( const std::string& name, bool on )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->topicOpOnly = on;
    return true;
}

// 클라이언트가 참여한 모든 채널에서 해당 클라이언트를 제거한다.
void ChannelRegistry::remove_Client_From_All_Channels( int fd )
{
    std::map<std::string, ChannelEntry>::iterator it = _channels.begin();

    while (it != _channels.end())
    {
        erase_Client_From_Channel(it->second, fd);

        if (it->second.members.empty())
        {
            std::map<std::string, ChannelEntry>::iterator toErase = it;
            ++it;
            _channels.erase(toErase);
            continue;
        }

        ensure_Channel_Has_Operator(it->second);

        ++it;
    }
}

// 채널의 현재 토픽을 반환한다.
std::string ChannelRegistry::get_Topic( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return "";
    return ch->topic;
}

// 채널 토픽을 새 값으로 설정한다.
bool ChannelRegistry::set_Topic( const std::string& name, const std::string& topic )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->topic = topic;
    return true;
}

// 지정한 클라이언트가 채널에 초대되어 있는지 확인한다.
bool ChannelRegistry::is_Invited( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    return ch->invited.find(fd) != ch->invited.end();
}

// 지정한 클라이언트를 채널 초대 목록에 추가한다.
bool ChannelRegistry::add_Invite( const std::string& name, int fd )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    std::pair<std::set<int>::iterator, bool> ret = ch->invited.insert(fd);
    return ret.second;
}

// 지정한 클라이언트를 채널 초대 목록에서 제거한다.
bool ChannelRegistry::remove_Invite( const std::string& name, int fd )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    return ch->invited.erase(fd) > 0;
}

// 채널에 키 모드가 설정되어 있는지 확인한다.
bool ChannelRegistry::has_Key( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return ch->hasKey;
}

// 채널 키가 설정되어 있으면 출력 인자로 복사한다.
bool ChannelRegistry::get_Key( const std::string& name, std::string& outKey ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch || !ch->hasKey)
        return false;

    outKey = ch->key;
    return true;
}

// 채널 키 모드를 켜고 키 값을 저장한다.
bool ChannelRegistry::set_Key( const std::string& name, const std::string& key )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->hasKey = true;
    ch->key = key;
    return true;
}

// 채널 키 모드를 끄고 키 값을 지운다.
bool ChannelRegistry::clear_Key( const std::string& name )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->hasKey = false;
    ch->key.clear();
    return true;
}

// 채널에 인원 제한이 설정되어 있는지 확인한다.
bool ChannelRegistry::has_User_Limit( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return ch->hasLimit;
}

// 채널 인원 제한이 설정되어 있으면 출력 인자로 복사한다.
bool ChannelRegistry::get_User_Limit( const std::string& name, size_t& outLimit ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch || !ch->hasLimit)
        return false;

    outLimit = ch->userLimit;
    return true;
}

// 채널 인원 제한 모드를 켜고 제한 값을 저장한다.
bool ChannelRegistry::set_User_Limit( const std::string& name, size_t limit )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->hasLimit = true;
    ch->userLimit = limit;
    return true;
}

// 채널 인원 제한 모드를 끄고 제한 값을 초기화한다.
bool ChannelRegistry::clear_User_Limit( const std::string& name )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->hasLimit = false;
    ch->userLimit = 0;
    return true;
}

// 채널이 설정된 인원 제한에 도달했는지 확인한다.
bool ChannelRegistry::is_Channel_Full( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    if (!ch->hasLimit)
        return false;

    return ch->members.size() >= ch->userLimit;
}

// 필요한 경우 채널을 만들고 클라이언트를 참여시킨다.
bool ChannelRegistry::join_Channel( const std::string& name, int fd )
{
    if (!has_Channel(name))
    {
        if (!add_Channel(name))
            return false;
    }

    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    if (is_Channel_Member(*ch, fd))
        return false;

    ch->members.insert(fd);
    ch->invited.erase(fd);

    ensure_Channel_Has_Operator(*ch);

    return true;
}

// 클라이언트가 채널 토픽을 변경할 수 있는지 확인한다.
bool ChannelRegistry::can_Change_Topic( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    if (!is_Channel_Member(*ch, fd))
        return false;
    if (!ch->topicOpOnly)
        return true;
    return is_Channel_Operator(*ch, fd);
}

// 클라이언트가 다른 사용자를 초대할 수 있는지 확인한다.
bool ChannelRegistry::can_Invite( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return has_Operator_Privilege(*ch, fd);
}

// 클라이언트가 다른 멤버를 강제 퇴장시킬 수 있는지 확인한다.
bool ChannelRegistry::can_Kick( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return has_Operator_Privilege(*ch, fd);
}

// 클라이언트가 채널 모드를 변경할 수 있는지 확인한다.
bool ChannelRegistry::can_Change_Mode( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return has_Operator_Privilege(*ch, fd);
}

// 채널에 참여 중인 모든 클라이언트 파일 디스크립터를 수집한다.
void ChannelRegistry::collect_Channel_Members( const std::string& name,
                                               std::vector<int>& outMembers ) const
{
    outMembers.clear();

    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return;

    std::set<int>::const_iterator it = ch->members.begin();
    for (; it != ch->members.end(); ++it)
        outMembers.push_back(*it);
}
