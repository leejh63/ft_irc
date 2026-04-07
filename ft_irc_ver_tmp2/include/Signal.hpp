#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <signal.h>

class Signal
{
private:
    static volatile sig_atomic_t _flag;

    static void handler( int sig );

public:
    static void setup( void );
    static int  getFlag( void );
    static void clearFlag( void );

private: // 금지
    Signal( void );
    ~Signal( void );
    Signal( const Signal& );
    Signal& operator=( const Signal& );
};

#endif