#ifndef _BEAN_STATE_H
#define _BEAN_STATE_H

#include "bzio.h"
#include "common.h"
#include "bobject.h"
#include "bhash.h"
#include "barray.h"

typedef struct Mbuffer Mbuffer;

typedef struct stringtable {
  struct TString ** hash;
  int nuse;
  int size;
} stringtable;

#define G(B)	(B->l_G)
#define CS(B)    (B->l_G->cScope)
#define GS(B)    (B->l_G->globalScope)
#define GCSV(B, key)   (hash_get(B, CS(B)->variables, key))
#define SCSV(B, key, value)   (hash_set(B, CS(B)->variables, key, value))
#define DCSV(B, key)   (hash_remove(B, CS(B)->variables, key))

#define SGSV(B, key, value)   (hash_set(B, GS(B)->variables, key, value))
#define GGSV(B, key)   (hash_get(B, GS(B)->variables, key))
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
  struct LexState * ls;
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
  TValue * tVal;
  TValue * fVal;
  TValue * nproto; // Prototype for Number
  TValue * sproto; // Prototype for String
  TValue * aproto; // Prototype for Array
  TValue * hproto; // Prototype for Hash
  TValue * mproto; // Prototype for Math
  TValue * dproto; // Prototype for Date
  Mbuffer * callStack;
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

typedef TValue * (*primitive_Fn) (bean_State * B, TValue * this, TValue * args, int argc);

typedef struct Tool {
  primitive_Fn function;
  TValue * context;
  bool getter;
} Tool;

typedef struct Function {
  Proto * p;
  dynamic_expr * body;
  TValue * context;
} Function;

typedef enum {
  EXPR_NIL,
  EXPR_NUM,
  EXPR_BOOLEAN,
  EXPR_UNARY,
  EXPR_CHANGE,
  EXPR_BINARY,
  EXPR_STRING,
  EXPR_FUN,
  EXPR_SELF,
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
    bean_Number nval;  /* for VKFLT */
    bu_byte bval;
    TString * sval;
    Function * fun;
    dynamic_expr * array;
    dynamic_expr * hash;

    struct {
      int op;
      struct expr * val;
    } unary;

    struct {
      int op;
      struct expr * val;
      int prefix;
    }  change;

    struct {
      int op; // Store the TokenType
      struct expr * left;  /* for   TK_ADD, TK_SUB, TK_MUL, TK_DIV, */
      struct expr * right;
      int assign; // Need reassign
    } infix;

    struct {
      TString * name;
      struct expr * value;
      bool global;
    } var;

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
        check_exp(novariant((o)->tt) == BEAN_TSTRING, &((cast_u(o))->ts))

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
#define shr(a, b) ((long)a >> (long)b)
#define shl(a, b) ((long)a << (long)b)
#define and(a, b) (a && b)
#define or(a, b) (a || b)
#define logic_and(a, b) ((long)a & (long)b)
#define logic_or(a, b) ((long)a | (long)b)
#define logic_xor(a, b) ((long)a ^ (long)b)
#define mod(a, b) ((long)a % (long)b)

#define BEAN_OK 1
#define BEAN_FAIL 0

void global_init(bean_State * B);
TValue * eval(bean_State * B, struct expr * expression);
void run_file(const char * path);
void run();
void enter_scope(bean_State * B);
void leave_scope(bean_State * B);

void call_stack_create_frame(bean_State * B, TValue * this);
void call_stack_restore_frame(bean_State * B);
char call_stack_peek(bean_State * B);
TValue * call_stack_peek_return(bean_State * B);
Tool * initialize_tool_by_fn(primitive_Fn fn, bool getter);
void set_prototype_function(bean_State *B, const char * method, uint32_t len, primitive_Fn fn, Hash * h);
void set_prototype_getter(bean_State *B, const char * method, uint32_t len, primitive_Fn fn, Hash * h);
void set_self_before_caling(bean_State * B, TValue * context);
#endif
