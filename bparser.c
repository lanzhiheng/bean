#include "blex.h"
#include "bparser.h"

/* maximum number of local variables per function (must be smaller
   than 250, due to the bytecode format) */
#define MAXVARS		200


static void error_expected (LexState *ls, int token) {
  beanX_syntaxerror(ls,
      beanO_pushfstring(ls->B, "%s expected", beanX_token2str(ls, token)));
}

/*
** Check that next token is 'c'.
*/
static void check (LexState *ls, int c) {
  if (ls->t.type != c)
    error_expected(ls, c);
}

/*
** Test whether next token is 'c'; if so, skip it.
*/
static int testnext (LexState *ls, int c) {
  if (ls->t.type == c) {
    beanX_next(ls);
    return true;
  }
  else return false;
}

/*
** Check that next token is 'c' and skip it.
*/
static void checknext (LexState *ls, int c) {
  check(ls, c);
  beanX_next(ls);
}


#define check_condition(ls, c, msg) { if(!c) beanX_syntaxerror(ls, msg); }

static TString *str_checkname(LexState *ls) {
  TString *ts;
  check(ls, TK_NAME);
  ts = ls -> t.seminfo.ts;
  beanX_next(ls);
  return ts;
}

static void init_exp(expdesc *e, expkind kind, int i) {
  e -> t = e -> f = NO_JUMP;
  e -> kind = kind;
  e -> u.info = i;
}

static void codestring(expdesc *e, TString *ts) {
  e -> kind = VKSTR;
  e -> u.strval = ts;
}

static void codename (LexState *ls, expdesc *e) {
  codestring(e, str_checkname(ls));
}
