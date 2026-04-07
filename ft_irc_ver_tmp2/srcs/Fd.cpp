#include "Fd.hpp"

#include <unistd.h> // close

Fd::Fd( void ) : _fd(-1) {}

Fd::Fd( int fd ) : _fd(fd) {}

Fd::~Fd( void )
{
    if (_fd >= 0) {
        close(_fd);
        _fd = -1;
    }
}

int Fd::get( void ) const { return _fd; }

bool Fd::valid( void ) const { return _fd >= 0; }

void Fd::reset( int new_fd )
{
    if (_fd >= 0) {
        close(_fd);
    }
    _fd = new_fd;
}