#include "Signal.hpp"

#include <cstring>


volatile sig_atomic_t Signal::_flag = 0;

// 받은 종료 시그널 번호를 전역 플래그에 저장한다.
void Signal::handler( int sig )
{
    _flag = sig;
}

// 서버가 안전하게 종료할 수 있도록 필요한 시그널 핸들러를 등록한다.
void Signal::setup( void )
{
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    // SIGPIPE는 끊긴 소켓에 쓸 때 프로세스가 종료되지 않도록 무시한다.
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);

    // 종료 요청 시그널은 플래그만 세우고 메인 루프에서 처리한다.
    sa.sa_handler = Signal::handler;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
}

// 마지막으로 받은 시그널 플래그를 반환한다.
int Signal::getFlag( void )
{
    return _flag;
}

// 시그널 플래그를 초기 상태로 되돌린다.
void Signal::clearFlag( void )
{
    _flag = 0;
}
