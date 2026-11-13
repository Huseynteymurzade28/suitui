#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
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

static void term_size(seal_state *S)
{
    struct winsize ws;

    seal_checkargc(S, 0);
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        ws.ws_col = 80;
        ws.ws_row = 24;
    }
    seal_newmap(S);
    seal_pushint(S, ws.ws_col);
    seal_setfield(S, -2, "w");
    seal_pushint(S, ws.ws_row);
    seal_setfield(S, -2, "h");
}

static void term_clear(seal_state *S)
{
    seal_checkargc(S, 0);
    fputs("\x1b[2J", stdout);
    seal_pushnull(S);
}

static void term_move(seal_state *S)
{
    seal_checkargc(S, 2);
    int x = seal_checkint(S, 0);
    int y = seal_checkint(S, 1);
    printf("\x1b[%d;%dH", y + 1, x + 1);
    seal_pushnull(S);
}

static void term_write(seal_state *S)
{
    seal_checkargc(S, 1);
    fputs(seal_checkstring(S, 0), stdout);
    seal_pushnull(S);
}

static void term_flush(seal_state *S)
{
    seal_checkargc(S, 0);
    fflush(stdout);
    seal_pushnull(S);
}

static void term_getch(seal_state *S)
{
    char c;

    seal_checkargc(S, 0);
    fflush(stdout);
    if (read(STDIN_FILENO, &c, 1) != 1) {
        seal_pushnull(S);
        return;
    }
    seal_pushlstring(S, &c, 1);
}

static const seal_reg termlib[] = {
    { "init",  term_init },
    { "quit",  term_quit },
    { "size",  term_size },
    { "clear", term_clear },
    { "move",  term_move },
    { "write", term_write },
    { "flush", term_flush },
    { "getch", term_getch },
    { NULL, NULL }
};

void sealopen_term(seal_state *S)
{
    seal_newlib(S, termlib);
    seal_pushconststring(S, SUITUI_VERSION);
    seal_setfield(S, -2, "version");
    seal_setglobal(S, "Term");
}
