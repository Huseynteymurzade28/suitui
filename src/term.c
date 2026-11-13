#include <stddef.h>
#include "seal.h"

#define SUITUI_VERSION "0.0.1"

static void term_hello(seal_state *S)
{
    seal_checkargc(S, 0);
    seal_pushconststring(S, "hello from C");
}

static const seal_reg termlib[] = {
    { "hello", term_hello },
    { NULL, NULL }
};

void sealopen_term(seal_state *S)
{
    seal_newlib(S, termlib);
    seal_pushconststring(S, SUITUI_VERSION);
    seal_setfield(S, -2, "version");
    seal_setglobal(S, "Term");
}
