#include "blex.h"
#include "bstring.h"
#include <limits.h>

/* ORDER RESERVED */
static const char *const beanX_tokens [] = {
    "and", "break", "do", "else", "elseif",
    "end", "false", "for", "function", "goto", "if",
    "in", "local", "nil", "not", "or", "repeat",
    "return", "then", "true", "until", "while",
    "//", "..", "...", "==", ">=", "<=", "~=",
    "<<", ">>", "::", "<eof>",
    "<number>", "<integer>", "<name>", "<string>"
};

void beanX_init(bean_State * L) {
  int i;
  TString * e = beanS_newString(L, BEAN_ENV);
}


void beanX_setinput (bean_State *B, LexState *ls, char * inputStream, TString *source,
                     int firstchar) {
  ls -> current = firstchar;
  ls -> linenumber = 1;
  ls -> lastline = 1;
  ls -> t.type = 0;
  ls -> lookahead.type = TK_EOS;
  ls -> source = source;
  ls -> fs = NULL;
  ls -> B = B;
  ls -> inputStream = inputStream;
  ls -> envn = beanS_newString(B, BEAN_ENV);
  beanZ_resizebuffer(B, ls -> buff, BEAN_MINBUFFER);
}

static void inclinenumber(LexState * ls) {
  int old = ls -> current;
  bean_assert(currIsNewline(ls));
  next(ls); // skip '\n' and '\r'
  if (currIsNewline(ls) && ls->current != old) next(ls);  // skip '\n\r' or '\r\n'
  if (++ls -> linenumber >= INT_MAX) LEX_ERROR(ls, "chunk has too many lines");
}

static int llex(LexState * ls, SemInfo * seminfo) {
  beanZ_resetbuffer(ls -> buff);

  while (true) {
    switch(ls -> current) {
    case '\n':
    case '\r': {
      inclinenumber(ls);
    }
    }
  }
}
