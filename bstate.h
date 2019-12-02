#ifndef _BEAN_STATE_H
#define _BEAN_STATE_H

#include "common.h"
#include "bobject.h"

typedef struct stringtable {
  struct TString ** hash;
  int nuse;
  int size;
} stringtable;

#define G(B)	(B->l_G)

/*
** Union of all collectable objects (only for conversions)
*/
union GCUnion {
  GCObject gc;  /* common header */
  struct TString ts;
  /* struct Udata u; */
  /* union Closure cl; */
  /* struct Table h; */
  /* struct Proto p; */
  /* struct lua_State th;  /\* thread *\/ */
  /* struct UpVal upv; */
};

typedef struct bean_State {
  size_t allocateBytes;
  struct global_State *l_G;
} bean_State;

typedef struct global_State {
  unsigned int seed;  /* randomized seed for hashes */
  GCObject *allgc;  /* list of all collectable objects */
  stringtable strt;
  struct Scope * globalScope;
  struct Scope * cScope;
} global_State;

typedef struct dynamic_expr {
  struct expr ** es;
  int size;
  int count;
} dynamic_expr;

typedef enum {
  EXPR_NUM,
  EXPR_FLOAT,
  EXPR_BOOLEAN,
  EXPR_BINARY,
  EXPR_FUN,
  EXPR_VAR,
  EXPR_CALL,
  EXPR_RETURN,
  EXPR_LOOP,
  EXPR_BRANCH
} EXPR_TYPE;

typedef struct expr {
  EXPR_TYPE type;
  union {
    bean_Integer ival;    /* for TK_INT */
    bean_Number nval;  /* for VKFLT */
    bu_byte bval;

    struct {
      int op; // Store the TokenType
      struct expr * left;  /* for   TK_ADD, TK_SUB, TK_MUL, TK_DIV, */
      struct expr * right;
    } infix;

    struct {
      TString * name;
    } var;

    struct {
      struct expr * ret_val;
    } ret;

    struct {
      TString * callee;
      dynamic_expr * args;
    } call;

    struct {
      struct expr * condition;
      dynamic_expr * body;
    } loop;

    struct {
      struct expr * condition;
      dynamic_expr * if_body;
      dynamic_expr * else_body;
    } branch;
  };
} expr;


#define cast_u(o)	cast(union GCUnion *, (o))

#define beanE_exitCcall(B)	((B)->nCcalls++)
#define beanE_enterCcall(B)	((B)->nCcalls--)

/* macros to convert a GCObject into a specific value */
#define gco2ts(o)  \
        check_exp(novariant((o)->tt) == LUA_TSTRING, &((cast_u(o))->ts))


#define add(a, b) (a + b)
#define sub(a, b) (a - b)
#define mul(a, b) (a * b)
#define div(a, b) (a / b)

void global_init(bean_State * B);
const char *beanO_pushfstring (bean_State *B UNUSED, const char *fmt, ...);
TValue * eval(bean_State * B, expr * expression);
void run_file(const char * path);
#endif
