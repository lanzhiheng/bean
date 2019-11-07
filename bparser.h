#ifndef BEAN_PARSER_H
#define BEAN_PARSER_H

#include "bobject.h"

/*
** Marks the end of a patch list. It is an invalid value both as an absolute
** address, and as a list link (would link an element to itself).
*/
#define NO_JUMP (-1)


/* TODO: Copy from lua source code, we can remove some of them */
/* kinds of variables/expressions */
typedef enum {
  VVOID,  /* when 'expdesc' describes the last expression a list,
             this kind means an empty list (so, no expression) */
  VNIL,  /* constant nil */
  VTRUE,  /* constant true */
  VFALSE,  /* constant false */
  VK,  /* constant in 'k'; info = index of constant in 'k' */
  VKFLT,  /* floating constant; nval = numerical float value */
  VKINT,  /* integer constant; ival = numerical integer value */
  VKSTR,  /* string constant; strval = TString address;
             (string is fixed by the lexer) */
  VNONRELOC,  /* expression has its value in a fixed register;
                 info = result register */
  VLOCAL,  /* local variable; var.ridx = local register;
              var.vidx = relative index in 'actvar.arr'  */
  VUPVAL,  /* upvalue variable; info = index of upvalue in 'upvalues' */
  VCONST,  /* compile-time constant; info = absolute index in 'actvar.arr'  */
  VINDEXED,  /* indexed variable;
                ind.t = table register;
                ind.idx = key's R index */
  VINDEXUP,  /* indexed upvalue;
                ind.t = table upvalue;
                ind.idx = key's K index */
  VINDEXI, /* indexed variable with constant integer;
                ind.t = table register;
                ind.idx = key's value */
  VINDEXSTR, /* indexed variable with literal string;
                ind.t = table register;
                ind.idx = key's K index */
  VJMP,  /* expression is a test/comparison;
            info = pc of corresponding jump instruction */
  VRELOC,  /* expression can put result in any register;
              info = instruction pc */
  VCALL,  /* expression is a function call; info = instruction pc */
  VVARARG  /* vararg expression; info = instruction pc */
} expkind;

#define vkisvar(k)	(VLOCAL <= (k) && (k) <= VINDEXSTR)
#define vkisindexed(k)	(VINDEXED <= (k) && (k) <= VINDEXSTR)

typedef struct BlockCnt {
  struct BlockCnt *previous;
  bu_byte nactvar;  /* # active locals outside the block */
  bu_byte upval;  /* true if some variable in the block is an upvalue */
} BlockCnt;

typedef struct expdesc {
  expkind kind;
  union {
    bean_Integer ival;    /* for VKINT */
    bean_Number nval;  /* for VKFLT */
    TString *strval;  /* for VKSTR */

    // TODO What is that
    int info;  /* for generic use */
    /* struct {  /\* for indexed variables *\/ */
    /*   short idx;  /\* index (R or "long" K) *\/ */
    /*   lu_byte t;  /\* table (register or upvalue) *\/ */
    /* } ind; */
    struct {  /* for local variables */
      bu_byte sidx;  /* index in the stack */
      unsigned short vidx;  /* index in 'actvar.arr'  */
    } var;
  } u;
  int t;  /* patch list of 'exit when true' */
  int f;  /* patch list of 'exit when false' */
} expdesc;

typedef struct FuncState {
  Proto *f;  /* current function header */
  struct FuncState * prev; /* enclosing func */
  struct LexState * ls;
  struct BlockCnt * bl;
  int pc;  /* next position to code (equivalent to 'ncode') */
  int firstlocal;  /* index of first local var (in Dyndata array) */
  bu_byte nactvar;  /* number of active local variables */
  bu_byte nups;  /* number of upvalues */
  short ndebugvars;  /* number of elements in 'f->locvars' */
  bu_byte needclose;  /* function needs to close upvalues when returning */
} FuncState;

/* kinds of variables */
#define VDKREG		0   /* regular */
#define RDKCONST	1   /* constant */
#define RDKTOCLOSE	2   /* to-be-closed */
#define RDKCTC		3   /* compile-time constant */

typedef union Vardesc {
  struct {
    TValuefields;
    bu_byte kind;
    bu_byte sidx; /* index of the variable in the stack */
    short pidx;  /* index of the variable in the Proto's 'locvars' array */
    TString *name;  /* variable name */
  } vd;

  // TODO: Why
  TValue k;  /* constant value (if any) */
} Vardesc;

typedef struct Dyndata {
  struct {
    Vardesc *arr;
    int n;
    int size;
  } actvar;
} Dyndata;

#endif
