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

    bool            inviteOnly;     // +i 초대 전용 모드
    bool            topicOpOnly;    // +t 오퍼레이터만 토픽 변경 가능

    bool            hasKey;         // +k 키 설정 여부
    std::string     key;            // +k 키 값

    bool            hasLimit;       // +l 인원 제한 설정 여부
    size_t          userLimit;      // +l 최대 인원 수
};

#endif
