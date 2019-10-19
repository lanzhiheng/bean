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

void beanX_init(bean_State * B) {
  int i;
  TString * e = beanS_newliteral(B, BEAN_ENV);
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
  ls -> envn = beanS_newliteral(B, BEAN_ENV);
  beanZ_resizebuffer(B, ls -> buff, BEAN_MINBUFFER);
}

static void inclinenumber(LexState * ls) {
  int old = ls -> current;
  bean_assert(currIsNewline(ls));
  next(ls); // skip '\n' and '\r'
  if (currIsNewline(ls) && ls->current != old) next(ls);  // skip '\n\r' or '\r\n'
  if (++ls -> linenumber >= INT_MAX) LEX_ERROR(ls, "chunk has too many lines");
}

static void read_string(LexState *ls, int del, SemInfo *seminfo) {
  save_and_next(ls);
  while(ls -> current != del) {
    switch(ls->current) {
      case EOZ:
        LEX_ERROR(ls, "unfinished string for end of file");
        break;  /* to avoid warnings */
      case '\n':
      case '\r':
        LEX_ERROR(ls, "unfinished string for line break");
        break;  /* to avoid warnings */
      case '\\': {
        int c;  /* final character to be saved */
        save_and_next(ls);  /* keep '\\' for error messages */

        switch(ls -> current) {
          case 'a': c = '\a'; goto read_save;
          case 'b': c = '\b'; goto read_save;
          case 'f': c = '\f'; goto read_save;
          case 'n': c = '\n'; goto read_save;
          case 'r': c = '\r'; goto read_save;
          case 't': c = '\t'; goto read_save;
          case 'v': c = '\v'; goto read_save;
          /* TODO: May be we can remove it. */
          /* case 'x': c = readhexaesc(ls); goto read_save; */
          /* case 'u': utf8esc(ls);  goto no_save; */
          case '\n': case '\r':
            inclinenumber(ls); c = '\n'; goto only_save;
          case '\\': case '\"': case '\'':
            c = ls->current; goto read_save;
          case EOZ: goto no_save;  /* will raise an error next loop */
          case 'z': {  /* zap following span of spaces */
            beanZ_buffremove(ls->buff, 1);  /* remove '\\' */
            next(ls);  /* skip the 'z' */
            while (bisspace(ls->current)) {
              if (currIsNewline(ls)) inclinenumber(ls);
              else next(ls);
            }
            goto no_save;
          }
          /* TODO: May be we can remove it. */
          /* default: { */
          /*   esccheck(ls, lisdigit(ls->current), "invalid escape sequence"); */
          /*   c = readdecesc(ls);  /\* digital escape '\ddd' *\/ */
          /*   goto only_save; */
          /* } */
        }
        read_save:
          next(ls);
        only_save:
          beanZ_buffremove(ls -> buff, 1);
          save(ls, c);
        no_save: break;
      }
      default:
        save_and_next(ls);
    }
  }
  save_and_next(ls);  /* skip delimiter */
  seminfo -> ts = beanX_newstring(ls, beanZ_buffer(ls -> buff) + 1, beanZ_bufflen(ls -> buff) - 2);
}

static int check_next1(LexState * ls, int c) {
  if (ls -> current == c) {
    next(ls);
    return true;
  }
  return false;
}

static int check_next2(LexState * ls, const char *set) {
  ASSERT(set[2] == '\0');
  if (ls->current == set[0] || ls->current == set[1]) {
    save_and_next(ls);
    return true;
  }
  else return false;
}

static const char *b_str2int (const char *s, bean_Integer *result) {
  const char * p = s;
  int base = 10;
  char * ptr;
  while (bisspace(cast_uchar(*p))) p++;  /* skip initial spaces */
  if (*p == '-' || * p == '+') p++;
  if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) base = 16;

  *result = strtol(s, &ptr, base);

  if (*ptr == '.') return NULL; // with pointer
  if (*result == 0 && (ptr - p + 1) > 1) return NULL; // parse error
  return ptr;
}

int luaO_str2num(char * s, TValue *o) {
  // TODO need to implement the string to number
  bean_Integer i; bean_Number n;
  const char *e;

  if ((e = b_str2int(s, &i)) != NULL) {  /* try as an integer */
  }

  return 0;
}

static int read_numeral(LexState * ls, SemInfo * seminfo) {
  TValue obj;
  const char *expo = "Ee";
  int first = ls -> current;
  ASSERT(bisdigit(ls->current));
  save_and_next(ls);

  if (first == '0' && check_next2(ls, "xX"))  /* hexadecimal? */
    expo = "Pp";
  while (true) {
    if (check_next2(ls, expo)) check_next2(ls, "+-");
    else if (bisxdigit(ls->current) || ls->current == '.')  /* '%x|%.' */
      save_and_next(ls);
    else break;
  }

  if (bislalpha(ls->current))  /* is numeral touching a letter? */
    save_and_next(ls);  /* force an error */
  save(ls, '\0');

  // TODO need to continue
  /* if (luaO_str2num(beanZ_buffer(ls -> buff), &obj) == 0) LEX_ERROR(ls, "malformed number"); */

  /* if (ttisinteger(&obj)) { */
  /*   seminfo->i = ivalue(&obj); */
  /*   return TK_INT; */
  /* } */
  /* else { */
  /*   lua_assert(ttisfloat(&obj)); */
  /*   seminfo->r = fltvalue(&obj); */
  /*   return TK_FLT; */
  /* } */
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
      case ' ': case '\f': case '\t': case '\v': {
        next(ls);
        break;
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
          while(ls -> current != EOZ) {
            int current = ls -> current;
            next(ls);

            if (current == '\n') inclinenumber(ls);
            if (current == '*' && ls -> current == '/') break;
          }
        }
        break;
      }
      case '"':
      case '\'': {
        read_string(ls, ls -> current, seminfo);
        return TK_STRING;
      }
      case '!': {
        next(ls);
        if (check_next1(ls, '=')) return TK_NE;
        else return '!';
      }
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': {
        /* return read_numeral(ls, seminfo); */
        printf("I am number");
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
