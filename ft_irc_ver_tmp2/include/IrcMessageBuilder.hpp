#ifndef IRCMESSAGEBUILDER_HPP
#define IRCMESSAGEBUILDER_HPP

#include <string>

#include "ClientRegistry.hpp"
#include "ChannelRegistry.hpp"

class IrcMessageBuilder
{
private:
    ClientRegistry&  _clients;
    ChannelRegistry& _channels;

public:
    IrcMessageBuilder( ClientRegistry& clients,
                       ChannelRegistry& channels );
    ~IrcMessageBuilder( void );

    std::string nick_Or_star( int fd ) const;

    std::string build_Welcome_Reply( const std::string& nick ) const;

    std::string build_Err_Need_more_params( const std::string& nick,
                                            const std::string& command ) const;
    std::string build_Err_Already_registered( const std::string& nick ) const;
    std::string build_Err_Password_mismatch( const std::string& nick ) const;
    std::string build_Err_Unknown_command( const std::string& nick,
                                           const std::string& command ) const;
    std::string build_Err_Not_registered( const std::string& nick ) const;
    std::string build_Err_Erroneus_nickname( const std::string& nick,
                                             const std::string& badNick ) const;
    std::string build_Err_Nickname_in_use( const std::string& nick,
                                           const std::string& badNick ) const;
    std::string build_Err_No_recipient( const std::string& nick,
                                        const std::string& command ) const;
    std::string build_Err_No_text_to_send( const std::string& nick ) const;
    std::string build_Err_No_such_nick( const std::string& nick,
                                        const std::string& targetNick ) const;
    std::string build_Err_No_such_channel( const std::string& nick,
                                           const std::string& channelName ) const;
    std::string build_Err_Cannot_send_to_chan( const std::string& nick,
                                               const std::string& channelName ) const;
    std::string build_Err_Not_on_channel( const std::string& nick,
                                          const std::string& channelName ) const;
    std::string build_Err_Chan_oprivs_needed( const std::string& nick,
                                              const std::string& channelName ) const;
    std::string build_Err_User_on_channel( const std::string& nick,
                                           const std::string& targetNick,
                                           const std::string& channelName ) const;
    std::string build_Err_Invite_only_chan( const std::string& nick,
                                            const std::string& channelName ) const;
    std::string build_Err_User_not_in_channel( const std::string& nick,
                                               const std::string& targetNick,
                                               const std::string& channelName ) const;
    std::string build_Err_Channel_is_full( const std::string& nick,
                                           const std::string& channelName ) const;
    std::string build_Err_Bad_channel_key( const std::string& nick,
                                           const std::string& channelName ) const;
    std::string build_Err_Unknown_mode( const std::string& nick,
                                        char modeChar ) const;
    std::string build_Err_Invalid_mode_param( const std::string& nick,
                                              const std::string& channelName,
                                              char modeChar,
                                              const std::string& param ) const;
    std::string build_Err_Last_operator( const std::string& nick,
                                         const std::string& channelName ) const;
    std::string build_Err_Umode_unknown_flag( const std::string& nick ) const;
    std::string build_Err_Users_dont_match( const std::string& nick ) const;

    std::string build_Rpl_Who_reply( const std::string& requesterNick,
                                     const std::string& channelName,
                                     int targetFd ) const;
    std::string build_Rpl_User_mode_is( const std::string& nick,
                                        const std::string& modes ) const;
    std::string build_Rpl_End_of_who( const std::string& requesterNick,
                                      const std::string& name ) const;
    std::string build_Rpl_Channel_mode_is( const std::string& nick,
                                           const std::string& channelName ) const;
    std::string build_Rpl_No_topic( const std::string& nick,
                                    const std::string& channelName ) const;
    std::string build_Rpl_Topic( const std::string& nick,
                                 const std::string& channelName,
                                 const std::string& topic ) const;
    std::string build_Rpl_Inviting( const std::string& nick,
                                    const std::string& targetNick,
                                    const std::string& channelName ) const;
    std::string build_Rpl_Name_reply( const std::string& nick,
                                      const std::string& channelName ) const;
    std::string build_Rpl_End_of_names( const std::string& nick,
                                        const std::string& channelName ) const;

    std::string build_Cap_Message( const std::string& target,
                                   const std::string& subCommand,
                                   const std::string& text ) const;
    std::string build_Pong_Message( const std::string& token ) const;
    std::string build_Join_Message( int fd,
                                    const std::string& channelName ) const;
    std::string build_Part_Message( int fd,
                                    const std::string& channelName,
                                    const std::string& reason ) const;
    std::string build_Quit_Message( int fd,
                                    const std::string& reason ) const;
    std::string build_Nick_Message( int fd,
                                    const std::string& newNick ) const;
    std::string build_Privmsg_Message( int fd,
                                       const std::string& target,
                                       const std::string& text ) const;
    std::string build_Topic_Message( int fd,
                                     const std::string& channelName,
                                     const std::string& topic ) const;
    std::string build_Invite_Message( int fd,
                                      const std::string& targetNick,
                                      const std::string& channelName ) const;
    std::string build_Kick_Message( int fd,
                                    const std::string& channelName,
                                    const std::string& targetNick,
                                    const std::string& comment ) const;
    std::string build_Mode_Message( int fd,
                                    const std::string& target,
                                    const std::string& modes,
                                    const std::string& param ) const;

private:
    std::string normalize_Target( const std::string& value ) const;
    std::string build_Server_Prefix( void ) const;
    std::string build_Reply( const std::string& code,
                             const std::string& target,
                             const std::string& params,
                             const std::string& text ) const;
    std::string build_User_Prefix( int fd ) const;
    std::string build_User_Command( int fd,
                                    const std::string& command,
                                    const std::string& params,
                                    const std::string& trailing,
                                    bool hasTrailing ) const;
    std::string build_Channel_Names( const std::string& channelName ) const;

private:
    IrcMessageBuilder( void );
    IrcMessageBuilder( const IrcMessageBuilder& );
    IrcMessageBuilder& operator=( const IrcMessageBuilder& );
};

#endif
