#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "seal.h"

static struct termios orig;
static int raw_on;
static char *on_restore;
static int saved_stderr = -1;
static FILE *errlog;

static void hold_stderr(void)
{
    if (!isatty(STDERR_FILENO) || !(errlog = tmpfile()))
        return;
    fflush(stderr);
    saved_stderr = dup(STDERR_FILENO);
    dup2(fileno(errlog), STDERR_FILENO);
}

static void release_stderr(void)
{
    char buf[4096];
    size_t n;

    if (saved_stderr == -1)
        return;
    fflush(stderr);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);
    saved_stderr = -1;
    rewind(errlog);
    while ((n = fread(buf, 1, sizeof(buf), errlog)) > 0)
        fwrite(buf, 1, n, stderr);
    fclose(errlog);
}

static void restore(void)
{
    if (!raw_on)
        return;
    if (on_restore)
        fputs(on_restore, stdout);
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
    release_stderr();
    raw_on = 0;
}

static void termio_raw(seal_state *S)
{
    static int registered;
    struct termios raw;

    seal_checkargcmax(S, 1);
    if (raw_on) {
        seal_pushnull(S);
        return;
    }
    if (tcgetattr(STDIN_FILENO, &orig) == -1) {
        seal_throw(S, "stdin is not a terminal");
        return;
    }
    free(on_restore);
    on_restore = seal_gettop(S) == 1 ? strdup(seal_checkstring(S, 0)) : NULL;

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
    hold_stderr();
    raw_on = 1;
    seal_pushnull(S);
}

static void termio_restore(seal_state *S)
{
    seal_checkargc(S, 0);
    restore();
    seal_pushnull(S);
}

static void termio_read(seal_state *S)
{
    struct pollfd p = { STDIN_FILENO, POLLIN, 0 };
    unsigned char c;
    int timeout = -1;

    seal_checkargcmax(S, 1);
    if (seal_gettop(S) == 1)
        timeout = seal_checkint(S, 0);
    fflush(stdout);
    if (poll(&p, 1, timeout) <= 0 || read(STDIN_FILENO, &c, 1) != 1) {
        seal_pushint(S, -1);
        return;
    }
    seal_pushint(S, c);
}

static void termio_write(seal_state *S)
{
    seal_checkargc(S, 1);
    fputs(seal_checkstring(S, 0), stdout);
    seal_pushnull(S);
}

static void termio_flush(seal_state *S)
{
    seal_checkargc(S, 0);
    fflush(stdout);
    seal_pushnull(S);
}

static void termio_size(seal_state *S)
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

static void termio_char(seal_state *S)
{
    char c;

    seal_checkargc(S, 1);
    c = seal_checkint(S, 0);
    seal_pushlstring(S, &c, 1);
}

static const seal_reg termiolib[] = {
    { "raw",     termio_raw },
    { "restore", termio_restore },
    { "read",    termio_read },
    { "write",   termio_write },
    { "flush",   termio_flush },
    { "size",    termio_size },
    { "char",    termio_char },
    { NULL, NULL }
};

void sealopen_termio(seal_state *S)
{
    seal_newlib(S, termiolib);
    seal_setglobal(S, "TermIO");
}
