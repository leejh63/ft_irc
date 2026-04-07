#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <cstdlib>
#include <cerrno>

std::string     white_trim( const std::string& str_word );
int             check_port( const char* port );
int             check_password( const char* password );

#endif