#include "ClientRegistry.hpp"

#include <algorithm>
#include <iostream>

namespace
{
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

    ClientEntry make_Client_Entry( int fd )
    {
        ClientEntry entry;

        entry.fd = fd;
        entry.inBuf.clear();
        entry.outBuf.clear();
        reset_Client_State(entry);

        return entry;
    }

    bool matches_Nick( const ClientEntry& entry, const std::string& nick )
    {
        return entry.hasNick && entry.nick == nick;
    }

    bool can_Register( const ClientEntry& entry )
    {
        return !entry.registered
            && entry.passOk
            && entry.hasNick
            && entry.hasUser;
    }

    void add_Mode_Flag( std::string& modes, char mode )
    {
        if (modes.find(mode) != std::string::npos)
            return;

        std::string::iterator pos = modes.begin();
        while (pos != modes.end() && *pos < mode)
            ++pos;

        modes.insert(pos, mode);
    }

    void remove_Mode_Flag( std::string& modes, char mode )
    {
        modes.erase(std::remove(modes.begin(), modes.end(), mode), modes.end());
    }
}

void ClientRegistry::debug_Print_All( void ) const
{
    std::cout << "\n====== CLIENT STATE ======\n";

    if (_clients.empty())
    {
        std::cout << "(no clients)\n";
        std::cout << "==========================\n";
        return;
    }

    for (std::map<int, ClientEntry>::const_iterator it = _clients.begin();
         it != _clients.end(); ++it)
    {
        const ClientEntry& entry = it->second;

        std::cout << "fd=" << entry.fd << "\n"
                  << " passOk=" << entry.passOk << "\n"
                  << " hasNick=" << entry.hasNick << "\n"
                  << " hasUser=" << entry.hasUser << "\n"
                  << " registered=" << entry.registered << "\n"
                  << " nick='" << entry.nick << "'\n"
                  << " user='" << entry.user << "'\n"
                  << " realName='" << entry.realName << "'\n"
                  << " userModes='" << entry.userModes << "'\n";
    }

    std::cout << "==========================\n";
}

ClientRegistry::ClientRegistry( void )
: _clients()
{
}

ClientRegistry::~ClientRegistry( void )
{
}

bool ClientRegistry::has_Client( int fd ) const
{
    return (_clients.find(fd) != _clients.end());
}

ClientEntry* ClientRegistry::find_By_Fd( int fd )
{
    std::map<int, ClientEntry>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return NULL;
    return &(it->second);
}

const ClientEntry* ClientRegistry::find_By_Fd( int fd ) const
{
    std::map<int, ClientEntry>::const_iterator it = _clients.find(fd);
    if (it == _clients.end())
        return NULL;
    return &(it->second);
}

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

bool ClientRegistry::add_Client( int fd )
{
    if (has_Client(fd))
        return false;

    _clients.insert(std::make_pair(fd, make_Client_Entry(fd)));
    return true;
}

bool ClientRegistry::remove_Client( int fd )
{
    std::map<int, ClientEntry>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return false;

    _clients.erase(it);
    return true;
}

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

bool ClientRegistry::is_Registered( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;

    return entry->registered;
}

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

bool ClientRegistry::set_Pass_Ok( int fd, bool value )
{
    ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;

    entry->passOk = value;
    return true;
}

bool ClientRegistry::set_Nick( int fd, const std::string& newNick )
{
    ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;

    entry->nick = newNick;
    entry->hasNick = !newNick.empty();
    return true;
}

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

bool ClientRegistry::has_Pass_Ok( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;
    return entry->passOk;
}

bool ClientRegistry::has_Nick( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;
    return entry->hasNick;
}

bool ClientRegistry::has_User( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return false;
    return entry->hasUser;
}

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

std::string ClientRegistry::get_User_Modes( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return "";
    return entry->userModes;
}

std::string ClientRegistry::get_Nick( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return "";
    return entry->nick;
}

std::string ClientRegistry::get_User( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return "";
    return entry->user;
}

std::string ClientRegistry::get_Real_Name( int fd ) const
{
    const ClientEntry* entry = find_By_Fd(fd);
    if (entry == NULL)
        return "";
    return entry->realName;
}
