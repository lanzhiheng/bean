#include <assert.h>
#include <math.h>
#include "bstring.h"
#include "berror.h"
#include "bparser.h"
#include "bmath.h"

static TValue * primitive_Math_ceil(bean_State * B, TValue * thisVal, TValue * args, int argc) {
  assert(argc >= 1);
  TValue * ret = malloc(sizeof(TValue));
  double val = nvalue(&args[0]);

  setnvalue(ret, ceil(val));
  return ret;
}

TValue * init_Math(bean_State * B) {
  global_State * G = B->l_G;
  TValue * proto = malloc(sizeof(TValue));
  Hash * h = init_hash(B);
  sethashvalue(proto, h);
  set_prototype_function(B, "ceil", 4, primitive_Math_ceil, hhvalue(proto));
  /* set_prototype_function(B, "toExponential", 13, primitive_Number_toExponential, hhvalue(proto)); */

  TValue * number = malloc(sizeof(TValue));
  setsvalue(number, beanS_newlstr(B, "Math", 4));
  hash_set(B, G->globalScope->variables, number, proto);

  return proto;
}
