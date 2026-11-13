#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "seal.h"

#define SUITUI_VERSION "0.0.1"

static struct termios orig;
static int active;

static void restore(void)
{
    if (!active)
        return;
    fputs("\x1b[0m\x1b[?25h\x1b[?1049l", stdout);
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
    active = 0;
}

static void term_init(seal_state *S)
{
    static int registered;
    struct termios raw;

    seal_checkargc(S, 0);
    if (active) {
        seal_pushnull(S);
        return;
    }
    if (tcgetattr(STDIN_FILENO, &orig) == -1) {
        seal_throw(S, "stdin is not a terminal");
        return;
    }

    raw = orig;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~OPOST;
    raw.c_cflag |= CS8;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    if (!registered) {
        atexit(restore);
        registered = 1;
    }
    fputs("\x1b[?1049h\x1b[?25l\x1b[2J", stdout);
    fflush(stdout);
    active = 1;
    seal_pushnull(S);
}

static void term_quit(seal_state *S)
{
    seal_checkargc(S, 0);
    restore();
    seal_pushnull(S);
}

static const seal_reg termlib[] = {
    { "init", term_init },
    { "quit", term_quit },
    { NULL, NULL }
};

void sealopen_term(seal_state *S)
{
    seal_newlib(S, termlib);
    seal_pushconststring(S, SUITUI_VERSION);
    seal_setfield(S, -2, "version");
    seal_setglobal(S, "Term");
}
