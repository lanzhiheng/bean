#include <stdarg.h>
#include "berror.h"
#include "bstring.h"

#define addstr(out, str, len) ( memcpy(out, str, len), out += (len) )

const char * beanO_pushfstring (bean_State *B UNUSED, const char *fmt, ...) {
  char * msg = malloc(MAX_STRING_BUFFER * sizeof(char));
  va_list argp;
  va_start(argp, fmt);
  vsnprintf(msg, MAX_STRING_BUFFER, fmt, argp); // Write the string to buffer
  va_end(argp);
  return msg;
}

static const char * beanG_addinfo(bean_State * B, const char * msg, TString * source, int line) {
  char buff[BEAN_IDSIZE];


  if (source) {
    if (tslen(source) <= BEAN_IDSIZE) {
      char * out = buff;
      addstr(out, getstr(source), tslen(source));
      *out = '\0';
    } else {
      memcpy(buff, getstr(source), BEAN_IDSIZE - 1);
      buff[BEAN_IDSIZE-1] = '\0';
    }
  } else {
    buff[0] = '?';
    buff[1] = '\0';
  }

  return beanO_pushfstring(B, "%s:%d %s", buff, line, msg);
}

void lex_error (LexState *ls, const char *msg, int token) {
  msg = beanG_addinfo(ls->B, msg, ls -> source, ls -> linenumber);
  if (token) {
    msg = beanO_pushfstring(ls->B, "%s near %s", msg, txtToken(ls, token));
  }

  printf("%s\n", msg);
  abort();
}

void syntax_error (LexState *ls, const char *msg) {
  lex_error(ls, msg, ls->t.type);
}

/* semantic error */
void semantic_error (LexState *ls, const char *msg) {
  ls->t.type = 0;  /* remove "near <token>" from final message */ // TODO: Why
  syntax_error(ls, msg);
}

#define COMMON_ERROR(errorType)                                         \
  do {                                                                  \
    LexState * ls = B->ls;                                              \
    char * msg = malloc(MAX_STRING_BUFFER * sizeof(char));              \
    va_list argp;                                                       \
    va_start(argp, fmt);                                                \
    vsnprintf(msg, MAX_STRING_BUFFER, fmt, argp);                 \
    va_end(argp);                                                 \
    printf("%s %s:%d %s\n", errorType, getstr(ls->source), ls->linenumber, msg);   \
    abort();                                    \
  } while(0)

void runtime_error (bean_State *B, const char *fmt, ...) {
  COMMON_ERROR("RuntimeError");
}

void io_error (bean_State *B, const char *fmt, ...) {
  COMMON_ERROR("IOError");
}

void mem_error (bean_State *B, const char *fmt, ...) {
  COMMON_ERROR("MemoryError");
}

void eval_error (bean_State *B, const char *fmt, ...) {
  COMMON_ERROR("EvalError");
}
