#ifndef IRC_CORE_HPP
#define IRC_CORE_HPP

#include <string>
#include <vector>

#include "ClientEntry.hpp"
#include "ClientRegistry.hpp"
#include "ChannelRegistry.hpp"
#include "IrcCommand.hpp"
#include "ServerAction.hpp"
#include "IrcMessageBuilder.hpp"

class IrcCore
{
private:
    enum handleResult
    {
        // join
        JOIN_OK,
        JOIN_ALREADY_MEMBER,
        JOIN_INVITE_ONLY,
        JOIN_BAD_KEY,
        JOIN_CHANNEL_FULL,

        // password
        PASS_ALREADY_REGISTERED,
        PASS_PARAM_MISSING,
        PASS_PASSWORD_OK,
        PASS_PASSWORD_BAD,

        // nickname
        NICK_PARAM_MISSING,
        NICK_ERRONEUS,
        NICK_IN_USE,
        NICK_OK,

        // user
        USER_PARAM_MISSING,
        USER_REALNAME_MISSING,
        USER_ALREADY_REGISTERED,
        USER_OK,

        // MODE
        MODE_APPLY_OK,
        MODE_PARAM_MISSING,
        MODE_NO_SUCH_NICK,
        MODE_USER_NOT_IN_CHANNEL,
        MODE_BAD_VALUE,
        MODE_UNKNOWN_CHAR,
        MODE_LAST_OPERATOR,

        // etc
        HANDLE_ERROR,
        HANDLE_UNKNOWN,
    };

    typedef void (IrcCore::*CommandHandler)( ClientEntry& entry,
                                             const IrcCommand& cmd,
                                             std::vector<ServerAction>& out );

    struct CommandRoute
    {
        const char*     verb;
        CommandHandler  handler;
        bool            allowedBeforeRegister;
    };

private:
    const std::string _server_password;
    ClientRegistry&   _clients;
    ChannelRegistry&  _channels;
    IrcMessageBuilder _messages;

public:
    IrcCore( const std::string& password,
             ClientRegistry& clients,
             ChannelRegistry& channels );

    ~IrcCore( void );

    void    disconnect_Client( int fd,
                               const std::string& reason,
                               std::vector<ServerAction>& out );

    void    handle_Line( ClientEntry& entry,
                         const std::string& raw_Line,
                         std::vector<ServerAction>& out );

private:
    void    push_Send( std::vector<ServerAction>& out,
                       int fd,
                       const std::string& message ) const;

    void    push_Close( std::vector<ServerAction>& out,
                        int fd ) const;

private: // 디버깅
    void        debug_Full( const ClientEntry& entry,
                            const IrcCommand& cmd,
                            const char* msg ) const;

    const char* debug_Message( handleResult result ) const;

private:
    handleResult    check_Pass( int fd, const IrcCommand& cmd ) const;
    handleResult    check_Nick( const IrcCommand& cmd ) const;
    handleResult    check_User( int fd, const IrcCommand& cmd ) const;
    handleResult    check_Join( const std::string& channelName,
                                int fd,
                                const std::string& providedKey ) const;

    handleResult    apply_Mode_Toggle( const std::string& channelName,
                                       char sign,
                                       char modeChar );

    handleResult    apply_Mode_Param( const std::string& channelName,
                                      char sign,
                                      char modeChar,
                                      const std::string& modeParam,
                                      std::string& appliedParam );

    handleResult    apply_Mode_Member( const std::string& channelName,
                                       char sign,
                                       char modeChar,
                                       const std::string& modeParam,
                                       std::string& appliedParam );

    handleResult    apply_Channel_Mode( const std::string& channelName,
                                        char sign,
                                        char modeChar,
                                        const std::string& modeParam,
                                        std::string& appliedParam );

    bool            apply_User_Mode( int fd,
                                     char sign,
                                     char modeChar );

    bool            mode_Requires_Param( char sign,
                                         char modeChar ) const;

    void            append_Mode_Change( std::string& outModes,
                                        std::string& outParams,
                                        char sign,
                                        char modeChar,
                                        const std::string& appliedParam ) const;

    void            try_Register( ClientEntry& entry,
                                  std::vector<ServerAction>& out );

    const CommandRoute*
                    find_Command_Route( const std::string& verb ) const;

    bool            ensure_Pass_Accepted( const ClientEntry& entry,
                                          const IrcCommand& cmd,
                                          std::vector<ServerAction>& out,
                                          const char* debugMessage );

    bool            require_Channel_Exists( const ClientEntry& entry,
                                            const IrcCommand& cmd,
                                            const std::string& channelName,
                                            std::vector<ServerAction>& out,
                                            const char* debugMessage );

    bool            require_Channel_Member( const ClientEntry& entry,
                                            const IrcCommand& cmd,
                                            const std::string& channelName,
                                            std::vector<ServerAction>& out,
                                            const char* debugMessage );

    bool            require_Channel_Privilege( const ClientEntry& entry,
                                               const IrcCommand& cmd,
                                               bool allowed,
                                               const std::string& channelName,
                                               std::vector<ServerAction>& out,
                                               const char* debugMessage ) const;

    bool            require_Target_Channel_Member( const ClientEntry& entry,
                                                   const IrcCommand& cmd,
                                                   const std::string& channelName,
                                                   const std::string& targetNick,
                                                   int targetFd,
                                                   std::vector<ServerAction>& out,
                                                   const char* debugMessage ) const;

    ClientEntry*    find_Target_Client( const ClientEntry& entry,
                                        const IrcCommand& cmd,
                                        const std::string& targetNick,
                                        std::vector<ServerAction>& out,
                                        const char* debugMessage );

    std::string     current_Nick( int fd ) const;

    void            reply_And_Debug( const ClientEntry& entry,
                                     const IrcCommand& cmd,
                                     std::vector<ServerAction>& out,
                                     const std::string& reply,
                                     const char* debugMessage ) const;

    void            reply_Need_More_Params( const ClientEntry& entry,
                                            const IrcCommand& cmd,
                                            std::vector<ServerAction>& out,
                                            const std::string& command,
                                            const char* debugMessage ) const;

    void            reply_Mode_Error( const ClientEntry& entry,
                                      const IrcCommand& cmd,
                                      std::vector<ServerAction>& out,
                                      const std::string& channelName,
                                      char modeChar,
                                      const std::string& modeParam,
                                      handleResult modeResult ) const;

    void    handle_Command( ClientEntry& entry,
                            const IrcCommand& cmd,
                            std::vector<ServerAction>& out );

    void    handle_Cap( ClientEntry& entry,
                        const IrcCommand& cmd,
                        std::vector<ServerAction>& out );

    void    handle_Ping( ClientEntry& entry,
                         const IrcCommand& cmd,
                         std::vector<ServerAction>& out );

    void    handle_Who( ClientEntry& entry,
                        const IrcCommand& cmd,
                        std::vector<ServerAction>& out );

    void    handle_Pong( ClientEntry& entry,
                         const IrcCommand& cmd,
                         std::vector<ServerAction>& out );

    void    handle_Error( ClientEntry& entry,
                          const IrcCommand& cmd,
                          std::vector<ServerAction>& out );

    void    handle_Pass( ClientEntry& entry,
                         const IrcCommand& cmd,
                         std::vector<ServerAction>& out );

    void    handle_Nick( ClientEntry& entry,
                         const IrcCommand& cmd,
                         std::vector<ServerAction>& out );

    void    handle_User( ClientEntry& entry,
                         const IrcCommand& cmd,
                         std::vector<ServerAction>& out );

    void    handle_Join( ClientEntry& entry,
                         const IrcCommand& cmd,
                         std::vector<ServerAction>& out );

    void    handle_Part( ClientEntry& entry,
                         const IrcCommand& cmd,
                         std::vector<ServerAction>& out );

    void    handle_Privmsg( ClientEntry& entry,
                            const IrcCommand& cmd,
                            std::vector<ServerAction>& out );

    void    handle_Topic( ClientEntry& entry,
                          const IrcCommand& cmd,
                          std::vector<ServerAction>& out );

    void    handle_Invite( ClientEntry& entry,
                           const IrcCommand& cmd,
                           std::vector<ServerAction>& out );

    void    handle_Kick( ClientEntry& entry,
                         const IrcCommand& cmd,
                         std::vector<ServerAction>& out );

    void    handle_User_Mode( ClientEntry& entry,
                              const IrcCommand& cmd,
                              std::vector<ServerAction>& out );

    void    handle_Mode( ClientEntry& entry,
                         const IrcCommand& cmd,
                         std::vector<ServerAction>& out );

    void    handle_Quit( ClientEntry& entry,
                         const IrcCommand& cmd,
                         std::vector<ServerAction>& out );

    void    handle_Unknown( ClientEntry& entry,
                            const IrcCommand& cmd,
                            std::vector<ServerAction>& out );
    bool    is_Nick_available( int fd, const std::string& nick ) const;
    bool    is_Number_String( const std::string& value ) const;
    bool    is_Supported_User_Mode( char modeChar ) const;
    bool    is_Supported_Channel_Mode( char modeChar ) const;

    void        send_To_Channel( const std::string& channelName,
                                 const std::string& message,
                                 std::vector<ServerAction>& out,
                                 int exceptFd ) const;

    void        send_To_Shared_Peers( int fd,
                                      const std::string& message,
                                      std::vector<ServerAction>& out ) const;

private: // 금지
    IrcCore( void );
    IrcCore( const IrcCore& );
    IrcCore& operator=( const IrcCore& );
};

#endif
