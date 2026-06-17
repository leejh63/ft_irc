#ifndef FD_HPP
#define FD_HPP

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


private:
    Fd( const Fd& );
    Fd& operator=( const Fd& );
};

#endif