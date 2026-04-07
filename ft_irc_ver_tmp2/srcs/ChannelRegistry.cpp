#include "ChannelRegistry.hpp"

#include <iostream> // 디버깅용

namespace
{
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

    bool is_Channel_Member( const ChannelEntry& channel, int fd )
    {
        return channel.members.find(fd) != channel.members.end();
    }

    bool is_Channel_Operator( const ChannelEntry& channel, int fd )
    {
        return channel.operators.find(fd) != channel.operators.end();
    }

    bool has_Operator_Privilege( const ChannelEntry& channel, int fd )
    {
        return is_Channel_Member(channel, fd) && is_Channel_Operator(channel, fd);
    }

    void erase_Client_From_Channel( ChannelEntry& channel, int fd )
    {
        channel.members.erase(fd);
        channel.operators.erase(fd);
        channel.invited.erase(fd);
    }

    bool should_Assign_Initial_Operator( const ChannelEntry& channel,
                                         bool created )
    {
        return created || channel.members.size() == 1;
    }
}

void ChannelRegistry::debug_Print_All( void ) const
{
    std::cout << "\n====== CHANNEL STATE ======\n";

    if (_channels.empty())
    {
        std::cout << "(no channels)\n";
        std::cout << "===========================\n";
        return;
    }

    for (std::map<std::string, ChannelEntry>::const_iterator it = _channels.begin();
         it != _channels.end(); ++it)
    {
        const ChannelEntry& ch = it->second;

        std::cout << "ChannelEntry: " << ch.name << "\n";
        std::cout << "topic: " << ch.topic << "\n";
        std::cout << "  Members: ";
        for (std::set<int>::const_iterator m = ch.members.begin();
             m != ch.members.end(); ++m)
            std::cout << *m << " ";
        std::cout << "\n";

        std::cout << "  Operators: ";
        for (std::set<int>::const_iterator o = ch.operators.begin();
             o != ch.operators.end(); ++o)
            std::cout << *o << " ";
        std::cout << "\n";

        std::cout << "  Invited(" << ch.invited.size() << "): ";
        for (std::set<int>::const_iterator it = ch.invited.begin();
            it != ch.invited.end(); ++it)
            std::cout << *it << " ";
        std::cout << "\n";

        std::cout << "  InviteOnly: " << (ch.inviteOnly ? "true" : "false") << "\n";
        std::cout << "  TopicOpOnly: " << (ch.topicOpOnly ? "true" : "false") << "\n";
        std::cout << "  hasKey: " << (ch.hasKey ? "true" : "false") << "\n";
        std::cout << "  key: " << (ch.key.empty()  ? "none" : ch.key) << "\n";
        std::cout << "  hasLimit: " << (ch.hasLimit ? "true" : "false") << "\n";
        std::cout << "  userLimit: " << ch.userLimit << "\n";
    }

    std::cout << "===========================\n";
}

ChannelRegistry::ChannelRegistry( void )
{
}

ChannelRegistry::~ChannelRegistry( void )
{
}

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

bool ChannelRegistry::has_Channel( const std::string& name ) const
{
    return _channels.find(name) != _channels.end();
}

ChannelEntry* ChannelRegistry::find_By_Name( const std::string& name )
{
    std::map<std::string, ChannelEntry>::iterator it = _channels.find(name);
    if (it == _channels.end())
        return NULL;
    return &it->second;
}

const ChannelEntry* ChannelRegistry::find_By_Name( const std::string& name ) const
{
    std::map<std::string, ChannelEntry>::const_iterator it = _channels.find(name);
    if (it == _channels.end())
        return NULL;
    return &it->second;
}

bool ChannelRegistry::add_Channel( const std::string& name )
{
    if (has_Channel(name))
        return false;

    _channels.insert(std::make_pair(name, make_Channel_Entry(name)));
    return true;
}

bool ChannelRegistry::remove_Channel( const std::string& name )
{
    return _channels.erase(name) > 0;
}

bool ChannelRegistry::remove_Channel_If_Empty( const std::string& name )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    if (!ch->members.empty())
        return false;

    return remove_Channel(name);
}

bool ChannelRegistry::has_Member( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    return is_Channel_Member(*ch, fd);
}

bool ChannelRegistry::add_Member( const std::string& name, int fd )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    std::pair<std::set<int>::iterator, bool> result = ch->members.insert(fd);
    return result.second;
}

bool ChannelRegistry::remove_Member( const std::string& name, int fd )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    if (!is_Channel_Member(*ch, fd))
        return false;

    erase_Client_From_Channel(*ch, fd);
    return true;
}

bool ChannelRegistry::remove_Member_And_Cleanup( const std::string& name, int fd )
{
    if (!remove_Member(name, fd))
        return false;

    remove_Channel_If_Empty(name);
    return true;
}

size_t ChannelRegistry::member_Count( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return 0;

    return ch->members.size();
}

bool ChannelRegistry::is_Operator( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    return is_Channel_Operator(*ch, fd);
}

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

bool ChannelRegistry::remove_Operator( const std::string& name, int fd )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    return ch->operators.erase(fd) > 0;
}

bool ChannelRegistry::would_Remove_Last_Operator( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    if (!is_Channel_Operator(*ch, fd))
        return false;

    return ch->operators.size() <= 1;
}

bool ChannelRegistry::is_Invite_Only( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return ch->inviteOnly;
}

bool ChannelRegistry::is_Topic_Op_Only( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return ch->topicOpOnly;
}

bool ChannelRegistry::set_Invite_Only( const std::string& name, bool on )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->inviteOnly = on;
    return true;
}

bool ChannelRegistry::set_Topic_Op_Only( const std::string& name, bool on )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->topicOpOnly = on;
    return true;
}

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

        ++it;
    }
}

std::string ChannelRegistry::get_Topic( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return "";
    return ch->topic;
}

bool ChannelRegistry::set_Topic( const std::string& name, const std::string& topic )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->topic = topic;
    return true;
}

bool ChannelRegistry::is_Invited( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    return ch->invited.find(fd) != ch->invited.end();
}

bool ChannelRegistry::add_Invite( const std::string& name, int fd )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    std::pair<std::set<int>::iterator, bool> ret = ch->invited.insert(fd);
    return ret.second;
}

bool ChannelRegistry::remove_Invite( const std::string& name, int fd )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    return ch->invited.erase(fd) > 0;
}

bool ChannelRegistry::has_Key( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return ch->hasKey;
}

bool ChannelRegistry::get_Key( const std::string& name, std::string& outKey ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch || !ch->hasKey)
        return false;

    outKey = ch->key;
    return true;
}

bool ChannelRegistry::set_Key( const std::string& name, const std::string& key )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->hasKey = true;
    ch->key = key;
    return true;
}

bool ChannelRegistry::clear_Key( const std::string& name )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->hasKey = false;
    ch->key.clear();
    return true;
}

bool ChannelRegistry::has_User_Limit( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return ch->hasLimit;
}

bool ChannelRegistry::get_User_Limit( const std::string& name, size_t& outLimit ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch || !ch->hasLimit)
        return false;

    outLimit = ch->userLimit;
    return true;
}

bool ChannelRegistry::set_User_Limit( const std::string& name, size_t limit )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->hasLimit = true;
    ch->userLimit = limit;
    return true;
}

bool ChannelRegistry::clear_User_Limit( const std::string& name )
{
    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    ch->hasLimit = false;
    ch->userLimit = 0;
    return true;
}

bool ChannelRegistry::is_Channel_Full( const std::string& name ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    if (!ch->hasLimit)
        return false;

    return ch->members.size() >= ch->userLimit;
}

bool ChannelRegistry::join_Channel( const std::string& name, int fd )
{
    bool created = false;

    if (!has_Channel(name))
    {
        if (!add_Channel(name))
            return false;
        created = true;
    }

    ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;

    if (is_Channel_Member(*ch, fd))
        return false;

    ch->members.insert(fd);
    ch->invited.erase(fd);

    if (should_Assign_Initial_Operator(*ch, created))
        ch->operators.insert(fd);

    return true;
}

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

bool ChannelRegistry::can_Invite( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return has_Operator_Privilege(*ch, fd);
}

bool ChannelRegistry::can_Kick( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return has_Operator_Privilege(*ch, fd);
}

bool ChannelRegistry::can_Change_Mode( const std::string& name, int fd ) const
{
    const ChannelEntry* ch = find_By_Name(name);
    if (!ch)
        return false;
    return has_Operator_Privilege(*ch, fd);
}

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