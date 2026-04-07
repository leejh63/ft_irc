#ifndef CHANNELREGISTRY_HPP
#define CHANNELREGISTRY_HPP

#include <map>
#include <vector>
#include <string>

#include "ChannelEntry.hpp"

class ChannelRegistry
{
public:
    ChannelRegistry( void );
    ~ChannelRegistry( void );

public: // 디버깅
    void    debug_Print_All( void ) const;

public:
    void                collect_Shared_Peers( int fd, std::set<int>& outPeers ) const;
    void                collect_User_Channels( int fd, std::vector<std::string>& outChannels ) const;

    bool                has_Channel( const std::string& name ) const;
    bool                add_Channel( const std::string& name );

    bool                remove_Channel( const std::string& name );
    bool                remove_Channel_If_Empty( const std::string& name );
    void                remove_Client_From_All_Channels( int fd );
    bool                remove_Member( const std::string& name, int fd );
    bool                remove_Member_And_Cleanup( const std::string& name, int fd );

    bool                has_Member( const std::string& name, int fd ) const;
    bool                add_Member( const std::string& name, int fd );
    size_t              member_Count( const std::string& name ) const;

    bool                is_Operator( const std::string& name, int fd ) const;
    bool                add_Operator( const std::string& name, int fd );
    bool                remove_Operator( const std::string& name, int fd );
    bool                would_Remove_Last_Operator( const std::string& name, int fd ) const;

    bool                is_Invite_Only(const std::string& name) const;
    bool                set_Invite_Only(const std::string& name, bool on);
    
    
    std::string         get_Topic( const std::string& name ) const;
    bool                set_Topic( const std::string& name, const std::string& topic );
    bool                is_Topic_Op_Only(const std::string& name) const;    
    bool                set_Topic_Op_Only(const std::string& name, bool on);

    bool                is_Invited( const std::string& name, int fd ) const;
    bool                add_Invite( const std::string& name, int fd );
    bool                remove_Invite( const std::string& name, int fd );

    bool                has_Key( const std::string& name ) const;
    bool                get_Key( const std::string& name, std::string& outKey ) const;
    bool                set_Key( const std::string& name, const std::string& key );
    bool                clear_Key( const std::string& name );

    bool                has_User_Limit( const std::string& name ) const;
    bool                get_User_Limit( const std::string& name, size_t& outLimit ) const;
    bool                set_User_Limit( const std::string& name, size_t limit );
    bool                clear_User_Limit( const std::string& name );
    bool                is_Channel_Full( const std::string& name ) const;
    
    bool                join_Channel( const std::string& name, int fd );

    bool                can_Change_Topic( const std::string& name, int fd ) const;
    bool                can_Invite( const std::string& name, int fd ) const;
    bool                can_Kick( const std::string& name, int fd ) const;
    bool                can_Change_Mode( const std::string& name, int fd ) const;

    void                collect_Channel_Members( const std::string& name, std::vector<int>& outMembers ) const;

private:
    ChannelEntry*       find_By_Name( const std::string& name );
    const ChannelEntry* find_By_Name( const std::string& name ) const;

private:
    std::map<std::string, ChannelEntry> _channels;
};

#endif