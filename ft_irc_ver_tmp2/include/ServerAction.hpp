#ifndef SERVER_ACTION_HPP
#define SERVER_ACTION_HPP

#include <string>

enum ServerActionType
{
    SERVER_ACTION_NONE = 0,
    SERVER_ACTION_SEND,
    SERVER_ACTION_CLOSE
};

struct ServerAction
{
    ServerActionType type;
    int           fd;
    std::string   message;
};

#endif