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

#define TV_MALLOC malloc(sizeof(TValue))

typedef struct bean_State {
  size_t allocateBytes;
  struct global_State *l_G;
  struct LexState * ls;
} bean_State;

typedef struct Scope {
  struct Scope *previous;
  Hash * variables;
} Scope;

typedef struct callStack {
  TValue * retVal;
  bool target;
  struct callStack * next;
} callStack;

typedef struct loopStack {
  TValue * retVal;
  bool target;
  struct loopStack * next;
} loopStack;

typedef struct global_State {
  unsigned int seed;  /* randomized seed for hashes */
  GCObject *allgc;  /* list of all collectable objects */
  stringtable strt;
  Scope * globalScope;
  Scope * cScope;
  TValue * nil;  // nil
  TValue * tVal;
  TValue * fVal;
  TValue * meta; // Prototype for Meta
  TValue * nproto; // Prototype for Number
  TValue * sproto; // Prototype for String
  TValue * aproto; // Prototype for Array
  TValue * hproto; // Prototype for Hash
  TValue * mproto; // Prototype for Math
  TValue * rproto; // Prototype for Regex
  TValue * dproto; // Prototype for Date
  TValue * fproto; // Prototype for Function
  TValue * netproto; // Prototype for Http
  callStack * callStack;
  loopStack * loopStack;
  Mbuffer * instructionStream;
} global_State;


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

typedef struct Fn {
  TString * name;
  size_t address;
  TValue * context;
} Fn;

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
void run_file(const char * path);
void run();
void enter_scope(bean_State * B);
void leave_scope(bean_State * B);

void call_stack_create_frame(bean_State * B);
void call_stack_restore_frame(bean_State * B);
char call_stack_peek(bean_State * B);
TValue * call_stack_peek_return(bean_State * B);
Tool * initialize_tool_by_fn(primitive_Fn fn, bool getter);
void set_prototype_function(bean_State *B, const char * method, uint32_t len, primitive_Fn fn, Hash * h);
void set_prototype_getter(bean_State *B, const char * method, uint32_t len, primitive_Fn fn, Hash * h);
void set_self_before_caling(bean_State * B, TValue * context);
void exception();
void assert_with_message(bool condition, char * message);
#endif
