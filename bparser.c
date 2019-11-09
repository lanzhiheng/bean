#include <limits.h>
#include "mem.h"
#include "blex.h"
#include "bstring.h"
#include "bparser.h"

/* maximum number of local variables per function (must be smaller
   than 250, due to the bytecode format) */
#define MAXVARS		200

#define hasmultret(kind)		((kind) == VCALL || (kind) == VVARARG)

/*
** maximum number of upvalues in a closure (both C and Lua). (Value
** must fit in a VM register.)
*/
#define MAXUPVAL	255

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
static bool testnext (LexState *ls, int c) {
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
  ts = ls -> t.seminfo.ts; // Get the seminfo from token structure
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
  checklimit(fs, dyd->actvar.n + 1 - fs->firstlocal, MAXVARS, "local variables");

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
  e->kind = VLOCAL;
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
  int stklevel = beanY_nvarstack(fs); // Return the number of variables in the stack for function 'fs'
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

static Upvaldesc *allocupvalue (FuncState *fs) {
  Proto * f = fs->f;
  int oldsize= f->sizeupvalues;
  checklimit(fs, fs->nups + 1, MAXUPVAL, "upvalues");
  beanM_growvector(fs->ls->B, f->upvalues, fs->nups, f->sizeupvalues, Upvaldesc, MAXUPVAL, "upvalues");
  while(oldsize < f->sizeupvalues) {
    f->upvalues[oldsize].name = NULL;
  }
  return &f->upvalues[fs->nups++];
}

static int newupvalue (FuncState *fs, TString *name, expdesc *v) {
  Upvaldesc *up = allocupvalue(fs);
  FuncState *prev = fs->prev;

  if (v->kind == VLOCAL) {
    up->instack = 1;
    up->idx = v->u.var.sidx;
    up->kind = getlocalvardesc(fs, v->u.var.vidx)->vd.kind;
    bean_assert(eqstr(name, getlocalvardesc(prev, v->u.var.vidx)->vd.name));
  } else {
    up->instack = 0;
    up->idx = cast_byte(v->u.info);
    up->kind = prev->f->upvalues[v->u.info].kind;
    bean_assert(eqstr(name, prev->f->upvalues[v->u.info].kind));
  }
  up->name = name;

  /* luaC_objbarrier(fs->ls->L, fs->f, name); */  // TODO: Why
  return fs->nups - 1;
}

/*
** Look for an active local variable with the name 'n' in the
** function 'fs'.
*/
static int searchvar(FuncState *fs, TString *n, expdesc *var) {
  for (int i = cast_int(fs->nactvar) - 1; i >= 0; i--) {
    Vardesc *vd = getlocalvardesc(fs, i);
    if (eqstr(n, vd->vd.name)) {
      if (vd->vd.kind == RDKCTC) {
        init_exp(var, VCONST, fs->firstlocal + i);
      } else {
        init_var(fs, var, i);
      }
      return var->kind;
    }
  }
  return -1;
}

/*
** Mark block where variable at given level was defined
** (to emit close instructions later).
*/
static void markupval (FuncState *fs, int level) {
  BlockCnt *bl = fs->bl;
  while (bl->nactvar > level)
    bl = bl->previous;
  bl->upval = true;
  fs->needclose = true;
}

/*
** Find a variable with the given name 'n'. If it is an upvalue, add
** this upvalue into all intermediate functions. If it is a global, set
** 'var' as 'void' as a flag.
*/
static void singlevaraux (FuncState *fs, TString *n, expdesc *var, int base) {
  if (fs == NULL) {
    init_exp(var, VVOID, 0); // global variable
  } else {
    int v = searchvar(fs, n, var);

    if (v >= 0) {
      if (v == VLOCAL && !base)
        markupval(fs, var->u.var.vidx);
    } else {
      int idx = searchupvalue(fs, n);
      if (idx < 0) {
        singlevaraux(fs->prev, n, var, 0);

        if (var->kind == VLOCAL || var->kind == VUPVAL) {
          idx = newupvalue(fs, n, var);
        } else {
          return;
        }
      }
      init_exp(var, VUPVAL, idx);
    }
  }
}

/*
** Find a variable with the given name 'n', handling global variables
** too.
*/

static void singlevar (LexState *ls, expdesc *var) {
  TString *varname = str_checkname(ls);
  FuncState *fs = ls->fs;
  singlevaraux(fs, varname, var, 1);

  if (var->kind == VVOID) {
    expdesc key;
    singlevaraux(fs, ls->envn, var, 1);
    bean_assert(var->k != VVOID);  /* this one must exist */
    codestring(&key, varname);
    // TODO: Need to implement the func
    /* luaK_indexed(fs, var, &key);  /\* env[varname] *\/ */
  }
}

/*
** Adjust the number of results from an expression list 'e' with 'nexps'
** expressions to 'nvars' values.
*/

/* static void adjust_assign (LexState *ls, int nvars, int nexps, expdesc *e) { */
/*   FuncState *fs = ls->fs; */
/*   int needed = nvars - nexps;  /\* extra values needed *\/ */

/*   if (hasmultret(e->kind)) {  /\* last expression has multiple returns? *\/ */
/*     int extra = needed + 1;  /\* discount last expression itself *\/ */
/*     if (extra < 0) { */
/*       extra = 0; */
/*     } */
/*     luaK_setreturns(fs, e, extra);  /\* last exp. provides the difference *\/ */
/*   } else { */
/*     if (e->kind != VVOID) { */
/*       luaK_exp2nextreg(fs, e);  /\* close last expression *\/ */
/*     } */

/*     if (needed > 0) { */
/*       luaK_nil(fs, fs->freereg, needed);  /\* complete with nils *\/ */
/*     } */
/*   } */

/*   if (needed > 0) { */
/*     luaK_reserveregs(fs, needed);  /\* registers for extra values *\/ */
/*   } else { /\* adding 'needed' is actually a subtraction *\/ */
/*     fs->freereg += needed;  /\* remove extra values *\/ */
/*   } */
/* } */

/*
** Macros to limit the maximum recursion depth while parsing
*/

#define enterlevel(ls)	beanE_enterCcall((ls)->B)
#define leavelevel(ls)	beanE_exitCcall((ls)->B)

/*
** Generates an error that a goto jumps into the scope of some
** local variable.
*/
static void jumpscopeerror(LexState *ls, Labeldesc *gt) {
  const char *varname = getstr(getlocalvardesc(ls->fs, gt->nactvar)->vd.name);
  const char *msg = "<goto %s> at line %d jumps into the scope of local '%s'";
  msg = beanO_pushfstring(ls->B, msg, getstr(gt->name), varname);
  beanK_semerror(ls, msg);
}


/*
** Solves the goto at index 'g' to given 'label' and removes it
** from the list of pending goto's.
** If it jumps into the scope of some variable, raises an error.
*/

/* static void solvegoto (LexState *ls, int g, Labeldesc *label) { */
/*   Labellist *gl = &ls->dyd->gt; */
/*   Labeldesc *gt = &gl->arr[g]; */
/*   bean_assert(eqstr(gt->name, label->name)); */
/*   if (gt->nactvar < label->nactvar) { /\* enter some scope? *\/ */
/*     jumpscopeerror(ls, gt); */
/*   } */
/*   luaK_patchlist(ls->fs, gt->pc, label->pc); // TODO: What it that? */
/*   for (int i = g; i<gl->n-1; i++) { */
/*     gl->arr[i] = gl->arr[i + 1]; */
/*   } */
/*   gl->n--; */
/* } */

/*
** Search for an active label with the given name.
*/
static Labeldesc *findlabel (LexState *ls, TString *name) {
  Dyndata * dyd = ls->dyd;

  for (int i = ls->fs->firstlabel; i < dyd->label.n; i++) {
    Labeldesc *ld = &dyd->label.arr[i];
    if (eqstr(ld->name, name)) {
      return ld;
    }
  }
  return NULL;
}

/*
** Adds a new label/goto in the corresponding list.
*/
static int newlabelentry (LexState *ls, Labellist *l, TString *name,
                          int line, int pc) {
  int n = l->n; // Num of label in Labellist
  beanM_growvector(ls->B, l->arr, n, l->size, Labeldesc, SHRT_MAX, "labels/gotos");
  l->arr[n].name = name;
  l->arr[n].line = line;
  l->arr[n].nactvar = ls->fs->nactvar;
  l->arr[n].close = 0;
  l->arr[n].pc = pc;
  l->n = n + 1;
  return n;
}

static int newgotoentry (LexState *ls, TString *name, int line, int pc) {
  return newlabelentry(ls, &ls->dyd->gt, name, line, pc);
}

void parse_statement(struct LexState *ls UNUSED) {};

static void parse_program(struct LexState * ls) {
  if (testnext(ls, TK_FUNCTION)) {
    printf("I will define the function\n");
  } else if (testnext(ls, TK_VAR)) {
    printf("I will define the variable\n");
  } else {
    parse_statement(ls);
  }
}


void bparser(struct LexState * ls) {
  while (ls -> current != '\0') {
    beanX_next(ls);
    const char * msg = txtToken(ls, ls -> t.type);
    parse_program(ls);
  }
}
