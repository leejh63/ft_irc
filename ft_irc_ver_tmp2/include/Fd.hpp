#ifndef FD_HPP
#define FD_HPP

// 소멸자를 통해 fd를 알아서 닫게하기 위한 클래스
class Fd {
private:
    int _fd;

public:
    Fd( void );
    Fd( int fd );
    ~Fd( void );

    int  get( void ) const;
    bool valid( void ) const;

    void reset( int new_fd );


private: // 금지
    Fd( const Fd& );
    Fd& operator=( const Fd& );
};

#endif