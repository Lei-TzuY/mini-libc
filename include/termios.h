#ifndef MINI_LIBC_TERMIOS_H
#define MINI_LIBC_TERMIOS_H

typedef unsigned char cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;

#define NCCS 19

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_line;
    cc_t c_cc[NCCS];
};

#define VINTR 0
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSUSP 10

#define ISIG 0x00001U
#define ICANON 0x00002U
#define ECHO 0x00008U

#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions,
              const struct termios *termios_p);

#endif
