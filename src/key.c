#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "key.h"

#define ESC_TIMEOUT 25

static int readbyte(int timeout)
{
    struct pollfd p = { STDIN_FILENO, POLLIN, 0 };
    unsigned char c;

    if (timeout >= 0 && poll(&p, 1, timeout) <= 0)
        return -1;
    if (read(STDIN_FILENO, &c, 1) != 1)
        return -1;
    return c;
}

static const char *tilde_name(int n)
{
    switch (n) {
    case 1: case 7: return "home";
    case 2:  return "insert";
    case 3:  return "delete";
    case 4: case 8: return "end";
    case 5:  return "pgup";
    case 6:  return "pgdown";
    case 11: return "f1";
    case 12: return "f2";
    case 13: return "f3";
    case 14: return "f4";
    case 15: return "f5";
    case 17: return "f6";
    case 18: return "f7";
    case 19: return "f8";
    case 20: return "f9";
    case 21: return "f10";
    case 23: return "f11";
    case 24: return "f12";
    }
    return NULL;
}

static const char *final_name(int c)
{
    switch (c) {
    case 'A': return "up";
    case 'B': return "down";
    case 'C': return "right";
    case 'D': return "left";
    case 'H': return "home";
    case 'F': return "end";
    case 'P': return "f1";
    case 'Q': return "f2";
    case 'R': return "f3";
    case 'S': return "f4";
    case 'Z': return "shift+tab";
    }
    return NULL;
}

static const char *read_csi(int *mod)
{
    int c, p[2] = { 0, 0 }, np = 0;

    while ((c = readbyte(ESC_TIMEOUT)) != -1) {
        if (c >= '0' && c <= '9')
            p[np] = p[np] * 10 + c - '0';
        else if (c == ';' && np == 0)
            np = 1;
        else
            break;
    }
    if (c == -1)
        return NULL;
    if (p[1] > 1)
        *mod = p[1] - 1;
    if (c == '~')
        return tilde_name(p[0]);
    return final_name(c);
}

static void with_mods(int mod, const char *name, char *out, size_t n)
{
    snprintf(out, n, "%s%s%s%s",
             mod & 4 ? "ctrl+" : "",
             mod & 2 ? "alt+" : "",
             mod & 1 ? "shift+" : "",
             name);
}

static void byte_name(int c, char *out, size_t n)
{
    switch (c) {
    case 0:   snprintf(out, n, "ctrl+space"); return;
    case 9:   snprintf(out, n, "tab");        return;
    case 10:
    case 13:  snprintf(out, n, "enter");      return;
    case 8:
    case 127: snprintf(out, n, "backspace");  return;
    case 27:  snprintf(out, n, "esc");        return;
    case ' ': snprintf(out, n, "space");      return;
    }
    if (c < 27)
        snprintf(out, n, "ctrl+%c", 'a' + c - 1);
    else if (c < 32)
        snprintf(out, n, "ctrl+%c", "\\]^_"[c - 28]);
    else
        snprintf(out, n, "%c", c);
}

static void utf8_char(int c, char *out, size_t n)
{
    int len = c >= 0xf0 ? 4 : c >= 0xe0 ? 3 : 2;
    int i;

    if (n < 5) {
        out[0] = '\0';
        return;
    }
    out[0] = c;
    for (i = 1; i < len; i++) {
        int b = readbyte(ESC_TIMEOUT);
        if (b == -1 || (b & 0xc0) != 0x80)
            break;
        out[i] = b;
    }
    out[i] = '\0';
}

int key_read(char *out, size_t n)
{
    const char *name = NULL;
    int mod = 0;
    int c = readbyte(-1);

    if (c == -1)
        return -1;
    if (c == 27) {
        int next = readbyte(ESC_TIMEOUT);
        if (next == '[')
            name = read_csi(&mod);
        else if (next == 'O')
            name = final_name(readbyte(ESC_TIMEOUT));
        else if (next != -1) {
            char ch[8];
            if (next >= 0xc0)
                utf8_char(next, ch, sizeof(ch));
            else
                byte_name(next, ch, sizeof(ch));
            if (strncmp(ch, "ctrl+", 5) == 0)
                snprintf(out, n, "ctrl+alt+%s", ch + 5);
            else
                snprintf(out, n, "alt+%s", ch);
            return 0;
        }
        if (next != -1 && !name)
            name = "unknown";
    }
    if (name)
        with_mods(mod, name, out, n);
    else if (c >= 0xc0)
        utf8_char(c, out, n);
    else
        byte_name(c, out, n);
    return 0;
}
