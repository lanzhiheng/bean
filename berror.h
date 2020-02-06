#ifndef _BEAN_ERRORS_H
#define _BEAN_ERRORS_H

#include "bstate.h"
#include "blex.h"

const char * beanO_pushfstring (bean_State *B UNUSED, const char *fmt, ...);
void lex_error (LexState *ls, const char *msg, int token);
void syntax_error (LexState *ls, const char *msg);
void runtime_error (bean_State *B, const char *fmt, ...);
void io_error (bean_State *B, const char *fmt, ...);
void mem_error (bean_State *B, const char *fmt, ...);
void eval_error (bean_State *B, const char *fmt, ...);

#endif
