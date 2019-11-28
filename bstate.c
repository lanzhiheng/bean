#include "bstate.h"
#include "blex.h"
#include "mem.h"
#include <stdio.h>
#include <stdarg.h>
#define MIN_STRT_SIZE 8

typedef TValue* (*eval_func) (bean_State * B, struct expr * expression);

static TValue * int_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * v = malloc(sizeof(TValue));
  v -> tt_ = BEAN_TNUMINT;
  v -> value_.i = expression -> ival;
  return v;
}

static TValue * float_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * v = malloc(sizeof(TValue));
  v -> tt_ = BEAN_TNUMFLT;
  v -> value_.n = expression -> nval;
  return v;
}

static TValue * boolean_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * v = malloc(sizeof(TValue));
  v -> tt_ = BEAN_TBOOLEAN;
  v -> value_.b = expression -> bval;
  return v;
}

static TValue * binary_eval (bean_State * B UNUSED, struct expr * expression) {
  TValue * v = malloc(sizeof(TValue));
  TokenType op = expression -> infix.op;
  TValue * v1 = eval(B, expression -> infix.left);
  TValue * v2 = eval(B, expression -> infix.right);
  bu_byte isfloat = ttisfloat(v1) || ttisfloat(v1);

  #define cal_statement(action) do {                      \
    if (isfloat) {                                      \
      v -> value_.n = action(nvalue(v1), nvalue(v2));   \
      v -> tt_ = BEAN_TNUMFLT;                          \
    } else {                                            \
      v -> value_.i = action(nvalue(v1), nvalue(v2));   \
      v -> tt_ = BEAN_TNUMINT;                          \
    }                                                   \
  } while(0)

  switch(op) {
    case(TK_ADD):
      cal_statement(add);
      break;
    case(TK_SUB):
      cal_statement(sub);
      break;
    case(TK_MUL):
      cal_statement(mul);
      break;
    case(TK_DIV):
      cal_statement(div);
      break;
    default:
      printf("need more code!");
  }

  #undef cal_statement

  return v;
}

/* static TValue * funcall_eval (bean_State * B UNUSED, struct expr * expression) {} */

eval_func fn[] = {
   int_eval,
   float_eval,
   boolean_eval,
   binary_eval
};


static stringtable stringtable_init(bean_State * B) {
  stringtable tb;
  tb.nuse = 0;
  tb.size = MIN_STRT_SIZE;
  tb.hash = beanM_mallocvchar(B, MIN_STRT_SIZE, TString *);

  for (int i = 0; i < MIN_STRT_SIZE; i++) {
    tb.hash[i] = NULL;
  }
  return tb;
}

void global_init(bean_State * B) {
  global_State * G = malloc(sizeof(global_State));
  G -> seed = rand();
  G -> allgc = NULL;
  G -> strt = stringtable_init(B);
  B -> l_G = G;
}

const char *beanO_pushfstring (bean_State *B UNUSED, const char *fmt, ...) {
  char * msg = malloc(MAX_STRING_BUFFER * sizeof(char));
  va_list argp;
  va_start(argp, fmt);
  vsnprintf(msg, MAX_STRING_BUFFER, fmt, argp); // Write the string to buffer
  va_end(argp);
  return msg;
}

TValue * eval(bean_State * B, struct expr * expression) {
  EXPR_TYPE t = expression -> type;
  return fn[t](B, expression);
}
