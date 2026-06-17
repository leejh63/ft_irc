#ifndef CLIENTREGISTRY_HPP
#define CLIENTREGISTRY_HPP

#include <map>
#include <string>

#include "ClientEntry.hpp"

class ClientRegistry
{
public:
    ClientRegistry( void );
    ~ClientRegistry( void );

public:
    void                debug_Print_All( void ) const;

    bool                has_Client( int fd ) const;

    ClientEntry*        find_By_Fd( int fd );
    const ClientEntry*  find_By_Fd( int fd ) const;

    ClientEntry*        find_By_Nick( const std::string& nick );
    const ClientEntry*  find_By_Nick( const std::string& nick ) const;

    bool                add_Client( int fd );
    bool                remove_Client( int fd );

    bool                is_Nick_In_Use( const std::string& nick,
                                        int exceptFd ) const;

    bool                is_Registered( int fd ) const;
    bool                try_Register( int fd );

    bool                set_Pass_Ok( int fd, bool value );
    bool                set_Nick( int fd, const std::string& newNick );
    bool                set_User( int fd,
                                  const std::string& user,
                                  const std::string& realName );
    bool                set_User_Mode( int fd,
                                       char mode,
                                       bool enabled );

    bool                has_Pass_Ok( int fd ) const;
    bool                has_Nick( int fd ) const;
    bool                has_User( int fd ) const;

    std::string         get_User_Modes( int fd ) const;
    std::string         get_Nick( int fd ) const;
    std::string         get_User( int fd ) const;
    std::string         get_Real_Name( int fd ) const;

private:
    std::map<int, ClientEntry> _clients;

private:
    ClientRegistry( const ClientRegistry& );
    ClientRegistry& operator=( const ClientRegistry& );
};

#endif
