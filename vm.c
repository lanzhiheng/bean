#include "vm.h"

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

eval_func fn[] = {
   int_eval,
   float_eval,
   boolean_eval,
   binary_eval
};

TValue * eval(bean_State * B, struct expr * expression) {
  EXPR_TYPE t = expression -> type;
  return fn[t](B, expression);
}
