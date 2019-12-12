#ifndef _BEAN_STATE_H
#define _BEAN_STATE_H

#include "common.h"
#include "bobject.h"
#include "bhash.h"
#include "barray.h"

typedef struct stringtable {
  struct TString ** hash;
  int nuse;
  int size;
} stringtable;

#define G(B)	(B->l_G)
#define CS(B)    (B->l_G->cScope)
#define GCSV(B, n)   (hash_get(B, CS(B)->variables, n))
#define SCSV(B, n, v)   (hash_set(B, CS(B)->variables, n, v))
#define DCSV(B, n)   (hash_remove(B, CS(B)->variables, n))
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

typedef struct Scope {
  struct Scope *previous;
  Hash * variables;
} Scope;

typedef struct global_State {
  unsigned int seed;  /* randomized seed for hashes */
  GCObject *allgc;  /* list of all collectable objects */
  stringtable strt;
  Scope * globalScope;
  Scope * cScope;
  TValue * nil;  // nil
  TValue * sproto; // Prototype for String
  TValue * aproto; // Prototype for Array
  TValue * hproto; // Prototype for Hash
} global_State;

typedef struct dynamic_expr {
  struct expr ** es;
  int size;
  int count;
} dynamic_expr;

typedef struct Proto {
  TString * name;
  TString ** args;
  bu_byte arity;
  bu_byte assign;
} Proto;

typedef TValue* (*primitive_Fn) (bean_State * B, TValue * this, struct expr * expression);

typedef struct Tool {
  primitive_Fn function;
} Tool;

typedef struct Function {
  Proto * p;
  dynamic_expr * body;
} Function;

typedef enum {
  EXPR_NIL,
  EXPR_NUM,
  EXPR_FLOAT,
  EXPR_BOOLEAN,
  EXPR_BINARY,
  EXPR_STRING,
  EXPR_FUN,
  EXPR_DVAR,
  EXPR_GVAR,
  EXPR_CALL,
  EXPR_RETURN,
  EXPR_LOOP,
  EXPR_BRANCH,
  EXPR_ARRAY,
  EXPR_HASH,
  EXPR_BREAK
} EXPR_TYPE;

typedef struct expr {
  EXPR_TYPE type;

  union {
    bean_Integer ival;    /* for TK_INT */
    bean_Number nval;  /* for VKFLT */
    bu_byte bval;
    TString * sval;
    Function * fun;
    dynamic_expr * array;
    dynamic_expr * hash;

    struct {
      int op; // Store the TokenType
      struct expr * left;  /* for   TK_ADD, TK_SUB, TK_MUL, TK_DIV, */
      struct expr * right;
    } infix;

    struct {
      TString * name;
      struct expr * value;
    } var;

    struct {
      TString * name;
    } gvar;

    struct {
      struct expr * ret_val;
    } ret;

    struct {
      struct expr * callee;
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

/* macros to convert a GCObject into a specific value */
#define gco2ts(o)  \
        check_exp(novariant((o)->tt) == LUA_TSTRING, &((cast_u(o))->ts))

#define add(a, b) (a + b)
#define sub(a, b) (a - b)
#define mul(a, b) (a * b)
#define div(a, b) (a / b)
#define eq(a, b) (a == b)
#define gte(a, b) (a >= b)
#define gt(a, b) (a > b)
#define lte(a, b) (a <= b)
#define lt(a, b) (a < b)
#define neq(a, b) (a != b)

void global_init(bean_State * B);
const char *beanO_pushfstring (bean_State *B UNUSED, const char *fmt, ...);
TValue * eval(bean_State * B, expr * expression);
void run_file(const char * path);
void enter_scope(bean_State * B);
void leave_scope(bean_State * B);
#endif
