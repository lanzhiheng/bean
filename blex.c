#include "blex.h"
#include "bstring.h"
#include <limits.h>

/* ORDER RESERVED */
const char *const beanX_tokens [] = {
    "and", "break", "else", "elseif",
    "+", "-", "*", "/",
    ":", ",", ";",
    "{", "}", "(", ")", "[", "]", ".",
    "false", "for", "fn", "if", "self",
    "in", "var", "nil", "not", "or",
    "return", "true",  "while",
    "==", "=", ">=", ">", "<=", "<", "~=",
    "<<", ">>", "<eof>",
    "<number>", "<integer>", "<name>", "<string>"
};

#define save_and_next(ls) (save(ls, ls->current), next(ls))

static void lexerror (LexState *ls, const char *msg, int token) {
  msg = luaG_addinfo(ls->B, msg, ls -> source, ls -> linenumber);
  if (token) {
    msg = beanO_pushfstring(ls->B, "%s near %s", msg, txtToken(ls, token));
  }

  printf("%s\n", msg);
  abort();
}

void beanX_syntaxerror (LexState *ls, const char *msg) {
  lexerror(ls, msg, ls->t.type);
}

static void save(LexState * ls, int c) {
  Mbuffer * b = ls -> buff;
  if (beanZ_bufflen(b) + 1 > beanZ_sizebuffer(b)) {
    size_t newsize;
    if (beanZ_sizebuffer(b) >= BUFFER_MAX/2) lexerror(ls, "lexical element too long", 0);
    newsize = beanZ_sizebuffer(b) * 2;
    beanZ_resizebuffer(ls -> B, b, newsize);
  }
  b -> buffer[beanZ_bufflen(b)++] = cast_char(c);
}

void beanX_init(bean_State * B) {
  TString * e = beanS_newliteral(B, BEAN_ENV);
  printf("BeanX initialize in %s.\n", getstr(e));
  /* luaC_fix(L, obj2gco(e));  /\* never collect this name *\/ */
  for (int i = 0; i < NUM_RESERVED; i++) {
    TString * ts = beanS_newlstr(B, beanX_tokens[i], strlen(beanX_tokens[i]));
    ts -> reserved = cast_byte(i + 1);
  }
}

const char *txtToken (LexState *ls, int token) {
  switch (token) {
    case TK_NAME: case TK_STRING:
    case TK_FLT: case TK_INT:
      save(ls, '\0');
      return beanO_pushfstring(ls->B, "'%s'", beanZ_buffer(ls->buff));
    default:
      return beanX_token2str(ls, token);
  }
}

const char *beanX_token2str (LexState *ls, int token) {
  const char *s = beanX_tokens[token];
  if (token < TK_EOS)  /* fixed format (symbols and reserved words)? */
    return beanO_pushfstring(ls->B, "'%s'", s);
  else  /* names, strings, and numerals */
    return s;
}

void beanX_setinput (bean_State *B, LexState *ls, char * inputStream, TString *source, int firstchar) {
  ls -> current = firstchar;
  ls -> linenumber = 1;
  ls -> lastline = 1;
  ls -> t.type = TK_EOS;
  ls -> pre.type = 0;
  ls -> source = source;
  ls -> fs = NULL;
  ls -> B = B;
  ls -> inputStream = inputStream;
  ls -> envn = beanS_newliteral(B, BEAN_ENV);
  ls -> buff = malloc(sizeof(Mbuffer));
  beanZ_initbuffer(ls -> buff);
}

/*
** creates a new string and anchors it in scanner's table so that
** it will not be collected until the end of the compilation
** (by that time it should be anchored somewhere)
*/
TString *beanX_newstring (LexState *ls, const char *str, size_t l) {
  bean_State *B = ls->B;
  TString *ts = beanS_newlstr(B, str, l);  /* create new string */

  // TODO: The VM logic
  /* TValue *o;  /\* entry for 'str' *\/ */

  /* setsvalue2s(L, L->top++, ts);  /\* temporarily anchor it in stack *\/ */
  /* o = luaH_set(L, ls->h, s2v(L->top - 1)); */
  /* if (isempty(o)) {  /\* not in use yet? *\/ */
  /*   /\* boolean value does not need GC barrier; */
  /*      table is not a metatable, so it does not need to invalidate cache *\/ */
  /*   setbvalue(o, 1);  /\* t[string] = true *\/ */
  /*   luaC_checkGC(L); */
  /* } */
  /* else {  /\* string already present *\/ */
  /*   ts = keystrval(nodefromval(o));  /\* re-use value previously stored *\/ */
  /* } */
  /* L->top--;  /\* remove string from stack *\/ */
  return ts;
}

static void inclinenumber(LexState * ls) {
  int old = ls -> current;
  bean_assert(currIsNewline(ls));
  next(ls); // skip '\n' and '\r'
  if (currIsNewline(ls) && ls->current != old) next(ls);  // skip '\n\r' or '\r\n'
  if (++ls -> linenumber >= INT_MAX) lexerror(ls, "chunk has too many lines", 0);
}

static void read_string(LexState *ls, int del, SemInfo *seminfo) {
  save_and_next(ls);
  while(ls -> current != del) {
    switch(ls->current) {
      case EOZ:
        lexerror(ls, "unfinished string for end of file", EOZ);
        break;  /* to avoid warnings */
      case '\n':
      case '\r':
        lexerror(ls, "unfinished string for line break", TK_STRING);
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
  bean_assert(set[2] == '\0');
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
  errno = 0;

  while (bisspace(cast_uchar(*p))) p++;  /* skip initial spaces */
  if (*p == '-' || * p == '+') p++;
  if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) base = 16;

  *result = strtol(s, &ptr, base);

  if (errno) return NULL; // parse error
  if (*ptr == '.') return NULL; // with pointer

  return ptr;
}

static const char *b_str2d (const char *s, bean_Number *result) {
  char * ptr;
  errno = 0;
  *result = strtod(s, &ptr);
  if (errno) return NULL; // parse error
  return ptr;
}

int beanO_str2num(char * s, TValue *o) {
  bean_Integer i; bean_Number n;
  const char *e;

  if ((e = b_str2int(s, &i)) != NULL) {  /* try as an integer */
    setivalue(o, i);
  } else if ((e = b_str2d(s, &n)) != NULL) {
    setfltvalue(o, n);
  } else {
    return 0;
  }

  return e - s;
}

static int read_numeral(LexState * ls, SemInfo * seminfo) {
  TValue obj;
  const char *expo = "Ee";
  int first = ls -> current;
  bean_assert(bisdigit(ls->current));
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

  if (beanO_str2num(beanZ_buffer(ls -> buff), &obj) == 0) lexerror(ls, "malformed number", TK_FLT);

  if (ttisinteger(&obj)) {
    seminfo->i = ivalue(&obj);
    return TK_INT;
  }
  else {
    bean_assert(ttisfloat(&obj));
    seminfo->r = fltvalue(&obj);
    return TK_FLT;
  }
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
      case '(': {
        next(ls);
        return TK_LEFT_PAREN;
      }
      case ')': {
        next(ls);
        return TK_RIGHT_PAREN;
      }
      case '[': {
        next(ls);
        return TK_LEFT_BRACKET;
      }
      case ']': {
        next(ls);
        return TK_RIGHT_BRACKET;
      }
      case '{': {
        next(ls);
        return TK_LEFT_BRACE;
      }
      case '}': {
        next(ls);
        return TK_RIGHT_BRACE;
      }
      case '+': {
        next(ls);
        return TK_ADD;
      }
      case ':': {
        next(ls);
        return TK_COLON;
      }
      case ',': {
        next(ls);
        return TK_COMMA;
      }
      case ';': {
        next(ls);
        return TK_SEMI;
      }
      case '.': {
        next(ls);
        return TK_DOT;
      }
      case '-': {
        next(ls);
        return TK_SUB;
      }
      case '*': {
        next(ls);
        return TK_MUL;
      }
      case '=': {
        next(ls);
        if (check_next1(ls, '=')) return TK_EQ;
        return TK_ASSIGN;
      }
      case '>': {
        next(ls);
        if (check_next1(ls, '=')) return TK_GE;
        else if (check_next1(ls, '>')) return TK_SHR;
        return TK_GT;
      }
      case '<': {
        next(ls);
        if (check_next1(ls, '=')) return TK_LE;
        else if (check_next1(ls, '<')) return TK_SHL;
        return TK_LT;
      }
      case '/': {
        next(ls);

        if (check_next1(ls, '/')) {
          // short comment
          while (!currIsNewline(ls) && ls->current != EOZ) next(ls);
        } else if (check_next1(ls, '*')){
          // long comment
          while(ls -> current != EOZ) {
            int pre = ls -> current;
            next(ls);

            if (pre == '\n') inclinenumber(ls);
            if (pre == '*' && check_next1(ls, '/')) break;
          }
        } else {
          return TK_DIV;
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
        return read_numeral(ls, seminfo);
      }
      case EOZ: {
        return TK_EOS;
      }
      default: {
        if (bislalpha(ls -> current)) {
          TString * ts;
          do {
            save_and_next(ls);
          } while(bislalnum(ls -> current) || ls -> current == '?'); // The name of variable or function can include '?'
          ts = beanX_newstring(ls,
                               beanZ_buffer(ls -> buff),
                               beanZ_bufflen(ls -> buff));
          seminfo->ts = ts;
          if (isreserved(ts)) {
            return ts -> reserved - 1;
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

void beanX_next (LexState *ls) {
  ls -> pre = ls -> t;
  ls -> t.type = llex(ls, &ls -> t.seminfo);
}
