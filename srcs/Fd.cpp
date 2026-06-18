#include "Fd.hpp"

#include <unistd.h>

// 비어 있는 파일 디스크립터 래퍼를 만든다.
Fd::Fd( void ) : _fd(-1) {}

// 전달받은 파일 디스크립터를 소유하는 래퍼를 만든다.
Fd::Fd( int fd ) : _fd(fd) {}

// 소유 중인 파일 디스크립터가 있으면 닫는다.
Fd::~Fd( void )
{
    if (_fd >= 0) {
        close(_fd);
        _fd = -1;
    }
}

// 현재 소유 중인 파일 디스크립터 값을 반환한다.
int Fd::get( void ) const { return _fd; }

// 유효한 파일 디스크립터를 소유 중인지 확인한다.
bool Fd::valid( void ) const { return _fd >= 0; }

// 기존 파일 디스크립터를 닫고 새 파일 디스크립터로 교체한다.
void Fd::reset( int new_fd )
{
    if (_fd >= 0) {
        close(_fd);
    }
    _fd = new_fd;
}
