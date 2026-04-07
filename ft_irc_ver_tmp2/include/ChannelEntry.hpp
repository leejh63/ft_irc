#ifndef CHANNELENTRY_HPP
#define CHANNELENTRY_HPP

#include <string>
#include <set>

struct ChannelEntry
{
    std::string     name;
    std::string     topic;

    std::set<int>   members;
    std::set<int>   operators;
    std::set<int>   invited;

    bool            inviteOnly;     // +i
    bool            topicOpOnly;    // +t

    bool            hasKey;         // +k on/off
    std::string     key;            // +k parameter

    bool            hasLimit;       // +l on/off
    size_t          userLimit;      // +l parameter
};

#endif