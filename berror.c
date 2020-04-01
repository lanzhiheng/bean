#include <stdarg.h>
#include "berror.h"
#include "bstring.h"
#include "bstate.h"

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
  exception();
}

void syntax_error (LexState *ls, const char *msg) {
  lex_error(ls, msg, ls->pre.type);
}

#define COMMON_ERROR(errorType, withNo)                                        \
  do {                                                                  \
    LexState * ls = B->ls;                                              \
    char * msg = malloc(MAX_STRING_BUFFER * sizeof(char));              \
    va_list argp;                                                       \
    va_start(argp, fmt);                                                \
    vsnprintf(msg, MAX_STRING_BUFFER, fmt, argp);                       \
    va_end(argp);                                                       \
    if (withNo) {                                                       \
      char * index = G(B)->instructionIndex;                               \
      int * pointer = G(B)->linenomap->buffer;                          \
      int lineNumber = pointer[index - G(B)->instructionStream->buffer]; \
      printf("%s: %s\n  from %s:%d\n", errorType, msg, getstr(ls->source), lineNumber); \
    } else {                                                            \
      printf("%s: %s\n  from %s\n", errorType, msg, getstr(ls->source)); \
    }                                                                 \
   exception();                                                      \
  } while(0)

void runtime_error (bean_State *B, const char *fmt, ...) {
  COMMON_ERROR("RuntimeError", true);
}

void io_error (bean_State *B, const char *fmt, ...) {
  COMMON_ERROR("IOError", false);
}

void mem_error (bean_State *B, const char *fmt, ...) {
  COMMON_ERROR("MemoryError", false);
}
