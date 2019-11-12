#ifndef BEAN_PARSER_H
#define BEAN_PARSER_H

#include "blex.h"
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

typedef enum {
  EXPR_NUM,
  EXPR_FLOAT,
  EXPR_BINARY,
  EXPR_FUN,
  EXPR_VAR
} EXPR_TYPE;

typedef struct expr {
  EXPR_TYPE type;
  union {
    bean_Integer ival;    /* for TK_INT */
    bean_Number nval;  /* for VKFLT */

    struct {
      const char * op;
      struct expr * left;  /* for   TK_ADD, TK_SUB, TK_MUL, TK_DIV, */
      struct expr * right;
    } infix;

    struct {
      TString * name;
    } var;
  };
} expr;

typedef struct Proto {
  TString * name;
  TString ** args;
  bu_byte arity;
} Proto;

typedef struct Function {
  Proto * p;
  expr ** body;
} Function;

typedef struct LexState LexState;

void bparser(struct LexState * ls);

#endif
