#include "vm.h"
#include "bfunction.h"
#include "bstring.h"
#include "bparser.h"

TValue * primitive_Function_call(bean_State * B UNUSED, TValue * thisVal UNUSED, TValue * args UNUSED, int argc UNUSED) {
  return NULL;
}

TValue * primitive_Function_apply(bean_State * B UNUSED, TValue * thisVal UNUSED, TValue * args UNUSED, int argc UNUSED) {
  return NULL;
}

TValue * init_Function(bean_State * B) {
  srand(time(0));
  global_State * G = B->l_G;
  TValue * proto = TV_MALLOC;
  Hash * h = init_hash(B);
  sethashvalue(proto, h);
  set_prototype_function(B, "call", 4, primitive_Function_call, hhvalue(proto));
  set_prototype_function(B, "apply", 5, primitive_Function_apply, hhvalue(proto));

  TValue * function = TV_MALLOC;
  setsvalue(function, beanS_newlstr(B, "Function", 8));
  hash_set(B, G->globalScope->variables, function, proto);

  return proto;
}
