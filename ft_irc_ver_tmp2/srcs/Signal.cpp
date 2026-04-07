#include "Signal.hpp"

#include <cstring>

#include <iostream>
#include <string>

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
    int sig_flag = _flag;
    if (!sig_flag) return sig_flag;
    // 임시 테스트용 서버쪽 가독성때문에 빼놓음
    if (sig_flag == SIGINT)      
        std::cout << "[signal] SIGINT received -> stopping...\n";
    else if (sig_flag == SIGTERM)
        std::cout << "[signal] SIGTERM received -> stopping...\n";
    else if (sig_flag == SIGQUIT)
        std::cout << "[signal] SIGQUIT received -> stopping...\n";
    else                    
        std::cout << "[signal] signal(" << sig_flag << ") received -> stopping...\n";

    return sig_flag;
}

void Signal::clearFlag( void )
{
    _flag = 0;
}