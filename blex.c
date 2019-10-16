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

#define save_and_next(ls) (save(ls, ls->current), next(ls))

static void save(LexState * ls, int c) {
  Mbuffer * b = ls -> buff;
  if (beanZ_bufflen(b) + 1 > beanZ_sizebuffer(b)) {
    size_t newsize;
    if (beanZ_sizebuffer(b) >= BUFFER_MAX/2) LEX_ERROR(ls, "lexical element too long");
    newsize = beanZ_sizebuffer(b) * 2;
    beanZ_resizebuffer(ls -> B, b, newsize);
  }
  b -> buffer[beanZ_bufflen(b)++] = cast_char(c);
}

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

static int check_next1(LexState * ls, int c) {
  if (ls -> current == c) {
    next(ls);
    return true;
  }
  return false;
}

static int llex(LexState * ls, SemInfo * seminfo) {
  beanZ_resetbuffer(ls -> buff);

  while (true) {
    switch(ls -> current) {
    case '\n':
    case '\r': {
      inclinenumber(ls);
      break;
    }
    case ' ', case '\f', case '\t', case '\v' {
      next(ls);
      break
    }
    case '=' {
      next(ls);
      if (check_next1(ls, '=')) return TK_EQ;
      return '=';
    }
    case '>' {
      next(ls);
      if (check_next1(ls, '=')) return TK_GE;
      else if (check_next1(ls, '>')) return TK_SHR;
      return '>';
    }
    case '<' {
      next(ls);
      if (check_next1(ls, '=')) return TK_LE;
      else if (check_next1(ls, '<')) return TK_SHL;
      return '<';
    }
    case '/': {
      next(ls);

      if (check_next1(ls, '/')) {
        // short comment
        while (!currIsNewline(ls) && ls->current != EOZ) next(ls);
      } else if (check_next1(ls, '*')){
        // long comment
        /* while (ls->current != EOZ && !check_next1(ls, '*')) { */
        /*   if (ls -> current == '\n') { */
        /*     inclinenumber(ls); */
        /*   } */
        /* } */
      }
    }
    case '"': {
      // TODO: 字符串
      printf("I am string")
    }
    case '!': {
      next(ls);
      if (check_next1(ls, '=')) return TK_NE;
      else return '!'
    }
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': {
      /* return read_numeral(ls, seminfo); */
      printf("I am number")
    }
    case EOZ: {
      return TK_EOS;
    }
    default: {
      if (bislalpha(ls -> current)) {
        do {
          save_and_next(ls);
        } while(bislalnum(ls -> current) || ls -> current == '?'); // The name of variable or function can include '?'
        TString * ts;
        if (isreserved(ts)) { // TODO: need to add logic
          return FIRST_RESERVED;
        } else {
          return TK_NAME;
        }
      } else {
        int c = ls->current;
        next(ls);
        return c;
      }
    }
    }
  }
}
