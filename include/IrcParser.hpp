#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

#include "IrcCommand.hpp"

class IrcParser {
public:
    static bool parse_Line( const std::string& raw_Line, IrcCommand& cmd );

private:
    static void parse_Stripcr( std::string& raw_String );
    static bool parse_Prefix( const std::string& raw_String, size_t& pos, std::string& out_Prefix );
    static bool parse_Verb( const std::string& raw_String, size_t& pos, std::string& out_Verb );
    static void parse_Params_Trailing( const std::string& raw_String,
                                       size_t pos,
                                       std::vector<std::string>& out_Params,
                                       std::string& out_Trailing,
                                       bool& out_HasTrailing );
private: // 금지
    IrcParser( void );
    ~IrcParser( void );
    IrcParser( const IrcParser& );
    IrcParser& operator=( const IrcParser& );
};

#endif