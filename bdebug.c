#include <stdarg.h>
#include "bdebug.h"
#include "bstring.h"

#define addstr(out, str, len) ( memcpy(out, str, len), out += (len) )

const char * luaG_addinfo(bean_State * B, const char * msg, TString * source, int line) {
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


void beanG_runerror (bean_State *B, const char *fmt, ...) {
  const char *msg;
  va_list argp;
  va_start(argp, fmt);
  msg = beanO_pushfstring(B, fmt, argp);  /* format message */
  va_end(argp);
  printf("%s", msg);
  abort();
}
