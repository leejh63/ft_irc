#include "Utils.hpp"

/* 
// white_trim
// 전달받은 문자열의 앞뒤에서 특정 문자(ws에 정의된 문자들)를 제거 후
// 새로운 문자열을 반환
// 문자열이 전부 공백일 경우 빈 문자열("")을 반환
*/
std::string white_trim(const std::string& str_word)
{

    static const char* ws = " \t\n\v\f\r";
    std::string::size_type start = str_word.find_first_not_of(ws);
    
    if (start == std::string::npos) return "";
    
    std::string::size_type last = str_word.find_last_not_of(ws);
    
    return str_word.substr(start, last - start + 1);
}

/*
// check_port
// 전달받은 문자열이 유효한 TCP 포트 번호 확인
// 조건:
//  1) NULL 아님
//  2) 앞뒤 공백 제거 후 비어있지 않음
//  3) 숫자로만 구성됨 (중간에 다른 문자 존재 불가)
//  4) long 범위 오버플로우 없음
//  5) 1 ~ 65535 범위 내 값
//
// 유효하면 해당 포트 번호(int)를 반환,
// 유효하지 않으면 0을 반환
*/

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

/*
// check_password
// 실행 인자로 전달되는 서버 비밀번호 확인
// 1) 비어 있지 않아야 함
// 2) 개행 문자를 포함하면 안 됨
// 유효 하면 1 반환 아니면 0 반환
*/
int check_password(const char* password)
{
    if (!password) return 0;
    std::string pass_string = password;
    if (pass_string.empty()) return 0;

    if (pass_string.find('\n') != std::string::npos) return 0;
    if (pass_string.find('\r') != std::string::npos) return 0;

    return 1;
}
