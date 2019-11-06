#include <limits.h>
#include "mem.h"
#include "blex.h"
#include "bstring.h>"
#include "bparser.h"

/* maximum number of local variables per function (must be smaller
   than 250, due to the bytecode format) */
#define MAXVARS		200

/* because all strings are unified by the scanner, the parser
   can use pointer equality for string equality */
#define eqstr(a,b)	((a) == (b))

/* semantic error */
void beanK_semerror (LexState *ls, const char *msg) {
  ls->t.type = 0;  /* remove "near <token>" from final message */ // TODO: Why
  beanX_syntaxerror(ls, msg);
}

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

/*
** Register a new local variable in the active 'Proto' (for debug
** information).
*/
static int registerlocalvar(LexState *ls, FuncState *fs, TString *varname) {
  Proto *f = fs->f;
  int oldsize = f->sizelocvars;
  beanM_growvector(ls->B, f->locvars, fs->ndebugvars, f->sizelocvars, LocVar, SHRT_MAX, "local variables");
  while (oldsize < f->sizelocvars) {
    f->locvars[oldsize].varname = NULL;
  }
  f->locvars[fs->ndebugvars].varname = varname;
  f->locvars[fs->ndebugvars].startpc = fs->pc;
  return fs->ndebugvars++;
}


static void errorlimit (FuncState *fs, int limit, const char *what) {
  bean_State *B = fs->ls->B;
  const char *msg;
  int line = fs->f->linedefined; // TODO: I need to confirm how to set it.
  const char *where = (line == 0)
                      ? "main function"
                      : beanO_pushfstring(B, "function at line %d", line);
  msg = beanO_pushfstring(B, "too many %s (limit is %d) in %s",
                             what, limit, where);
  beanX_syntaxerror(fs->ls, msg);
}


static void checklimit (FuncState *fs, int v, int l, const char *what) {
  if (v > l) errorlimit(fs, l, what);
}

/*
** Create a new local variable with the given 'name'. Return its index
** in the function.
*/

static int new_localvar (LexState * ls, TString *name) {
  bean_State * B = ls -> B;
  FuncState *fs = ls -> fs;
  Dyndata * dyd = ls -> dyd;
  Vardesc * var;
  checklimit(fs, dyd->actvar.n, MAXVARS, "local variables");

  beanM_growvector(B, dyd->actvar.arr, dyd->actvar.n + 1,
                   dyd->actvar.size, Vardesc, USHRT_MAX, "local variables");

  var = &dyd->actvar.arr[dyd->actvar.n++];
  var->vd.kind = VDKREG;
  var->vd.name = name;
  return dyd->actvar.n - 1 - fs-> firstlocal;
}

#define new_localvarliteral(ls, v) \
  new_localvar(ls, beanX_newstring(ls, "" v, (sizeof(v)/sizeof(char)) - 1))


/*
** Return the "variable description" (Vardesc) of a given
** variable
*/
static Vardesc *getlocalvardesc (FuncState *fs, int i) {
  return &fs->ls->dyd->actvar.arr[fs->firstlocal + i];
}

/*
** Convert 'nvar' (number of active variables at some point) to
** number of variables in the stack at that point.
*/

static int stacklevel(FuncState * fs, int nvar) {
  while (nvar > 0) {
    Vardesc * vardesc = getlocalvardesc(fs, nvar-1);

    if (vardesc -> vd.kind != RDKCTC) {
      return vardesc -> vd.sidx + 1;
    } else {
      nvar --;
    }
  }
  return 0;
}

/*
** Return the number of variables in the stack for function 'fs'
*/
int beanY_nvarstack (FuncState *fs) {
  return stacklevel(fs, fs->nactvar);
}


/*
** Get the debug-information entry for current variable 'i'.
*/
static LocVar * localdebuginfo(FuncState * fs, int i) {
  Vardesc * vardesc = getlocalvardesc(fs, i);
  if (vardesc->vd.kind == RDKCTC) {
    return NULL;
  } else {
    int idx = vardesc->vd.pidx;
    bean_assert(idx < fs->ndebugvars);
    return &fs->f->locvars[idx];
  }
}

static void init_var(FuncState * fs, expdesc * e, int i) {
  e->t = e->f = NO_JUMP;
  e->u.var.vidx = i;
  e->u.var.sidx = getlocalvardesc(fs, i) -> vd.sidx;
}

static void check_readonly (LexState *ls, expdesc *e) {
  FuncState * fs = ls -> fs;
  TString *varname = NULL;

  switch (e->kind) {
    case VCONST: {
      varname = ls->dyd->actvar.arr[e->u.info].vd.name;
    }
    case VLOCAL: {
      Vardesc * vardesc = getlocalvardesc(fs, e->u.var.vidx);
      if (vardesc -> vd.kind != VDKREG)
        varname = vardesc -> vd.name;
      break;
    }
    case VUPVAL: {
      Upvaldesc *up = &fs->f->upvalues[e->u.info];
      if (up->kind != VDKREG)
        varname = up->name;
      break;
    }
    default:
      return;
  }

  if (varname) {
    const char * msg = beanO_pushfstring(ls->B, "attempt to assign to const variable '%s'", getstr(varname));
    beanK_semerror(ls, msg);
  }
}

/*
** Start the scope for the last 'nvars' created variables.
*/

static void adjustlocalvars(LexState *ls, int nvars) {
  FuncState * fs = ls->fs;
  int stklevel = beanY_nvarstack(fs);
  int i;

  for (i = 0; i < nvars; i++) {
    int varidx = fs->nactvar++;
    Vardesc * var = getlocalvardesc(fs, varidx);
    var->vd.sidx = stklevel++;
    var->vd.pidx = registerlocalvar(ls, fs, var->vd.name);
  }
}

/*
** Close the scope for all variables up to level 'tolevel'.
** (debug info.)
*/
static void removevars (FuncState * fs, int tolevel) {
  fs->ls->dyd->actvar.n -= (fs->nactvar - tolevel);

  while (fs->nactvar > tolevel) {
    LocVar *var = localdebuginfo(fs, --fs->nactvar);
    if (var)
      var->endpc = fs->pc;
  }
}

/*
** Search the upvalues of the function 'fs' for one
** with the given 'name'.
*/
static int searchupvalue (FuncState * fs, TString * name) {
  Upvaldesc * upvalues = fs->f->upvalues;

  for (int i = 0; i < fs->nups; i++) {
    if (eqstr(upvalues[i].name, name)) return i;
  }

  return -1;
}
