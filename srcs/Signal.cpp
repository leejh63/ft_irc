#include "Signal.hpp"

#include <cstring>


volatile sig_atomic_t Signal::_flag = 0;

void Signal::handler( int sig )
{
    _flag = sig;
}

void Signal::setup( void )
{
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    // SIGPIPE ignore
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);

    // SIGINT / SIGTERM / SIGQUIT
    sa.sa_handler = Signal::handler;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    //sigaction(SIGPIPE, &sa, NULL);
}

int Signal::getFlag( void )
{
    return _flag;
}

void Signal::clearFlag( void )
{
    _flag = 0;
}