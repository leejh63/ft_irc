#include "Utils.hpp"

// 문자열 양끝의 공백 문자를 제거한다.
std::string white_trim(const std::string& str_word)
{

    static const char* ws = " \t\n\v\f\r";
    std::string::size_type start = str_word.find_first_not_of(ws);
    
    if (start == std::string::npos) return "";
    
    std::string::size_type last = str_word.find_last_not_of(ws);
    
    return str_word.substr(start, last - start + 1);
}

// 포트 문자열을 검증하고 사용 가능한 포트 번호로 변환한다.
int check_port(const char* port)
{
    if (!port) return 0;
    std::string port_string = white_trim(port);
    if (port_string.empty()) return 0;

    char* endptr = NULL;
    errno = 0;
    long value = std::strtol(port_string.c_str(), &endptr, 10);

    if (errno == ERANGE) return 0;
    if (*endptr != '\0') return 0;
    if (value < 1 || value > 65535) return 0;

    return static_cast<int>(value);
}

// 서버 비밀번호가 비어 있거나 개행을 포함하지 않는지 확인한다.
int check_password(const char* password)
{
    if (!password) return 0;
    std::string pass_string = password;
    if (pass_string.empty()) return 0;

    if (pass_string.find('\n') != std::string::npos) return 0;
    if (pass_string.find('\r') != std::string::npos) return 0;

    return 1;
}
