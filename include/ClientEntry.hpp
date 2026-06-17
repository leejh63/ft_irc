#ifndef CLIENTENTRY_HPP
#define CLIENTENTRY_HPP

#include <string>

struct ClientEntry
{
    int         fd;
    std::string inBuf;
    std::string outBuf;

    bool        passOk;
    bool        hasNick;
    bool        hasUser;
    bool        registered;

    std::string nick;
    std::string user;
    std::string realName;
    std::string userModes;
};

#endif
